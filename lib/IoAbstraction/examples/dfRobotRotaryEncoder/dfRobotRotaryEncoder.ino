/**
 * This is another example of using the DFRobot library, this time in conjunction with it's display. For the
 * simplest possible example, see dfRobotAnalogInSwitches example.
 * 
 * It expects that you include a liquid crystal library that you have available. I will assume you are using
 * the default one that's shipped with the IDE.
 * 
 * Shows the value of a rotary encoder on the display based on the UP and DOWN buttons. Select is also handled.
 *
 * Documentation and reference:
 *
 * https://www.thecoderscorner.com/products/arduino-downloads/io-abstraction/
 * https://www.thecoderscorner.com/ref-docs/ioabstraction/html/index.html
 */

// note you can switch this to include <LiquidCrystal.h> instead, just change the construction of lcd too.
#include <LiquidCrystalIO.h>
#include <IoAbstraction.h>
#include <DfRobotInputAbstraction.h>
#include <TaskManagerIO.h>
#include <TextUtilities.h>

// As per the above wiki this uses the default settings for analog ranges.
DfRobotInputAbstraction dfRobotKeys(dfRobotAvrRanges);
//DfRobotInputAbstraction dfRobotKeys(dfRobotV1AvrRanges, );

// Liquid crystal has an empty constructor for DfRobot.
LiquidCrystal lcd;
EncoderUserIntention intent = CHANGE_VALUE;

// Here we use a task manager event to manage the screen.
// This event is triggered when the encoder changes, never faster than 250ms.
// If there are changes to be painted it will re-paint the rotary encoder value.
// Never attempt to repaint something to an LCD too frequently, they are very slow.
//
class PaintEvent : public BaseEvent {
public:
    enum PaintButtonState { NOT_PRESSED, PRESSED, BUTTON_HELD};
private:
    PaintButtonState selState;
    int encoderReading;
    pinid_t cursorKey;
public:
    uint32_t timeOfNextCheck() override {
        // we are using a polled event, so that we restrict it's execution to about 4 times a second.
        return 250UL * 1000UL;
    }

    void exec() override {
        //
        // set up the display and print our title on the first line
        //
        lcd.begin(16, 2);
        lcd.setCursor(0, 0);
        lcd.print("encoder: ");
        const char* intentStr = "Chg Val";
        if(intent == SCROLL_THROUGH_ITEMS) intentStr = "Scroll";
        if(intent == SCROLL_THROUGH_SIDEWAYS) intentStr = "ScSide";
        lcd.print(intentStr);

        // zero pad a numeric value to four letters and print it.
        char sz[10];
        ltoaClrBuff(sz, encoderReading, 4, '0', sizeof sz);
        lcd.setCursor(0, 1);
        lcd.print(sz);

        // now we print the select button state into right corner.
        const char* btnState = "     ";
        if(selState == PRESSED) btnState = "PRESS";
        else if(selState == BUTTON_HELD) btnState = "HELD ";
        lcd.setCursor(12, 1);
        lcd.print(btnState);

        const char* cursorState = "     ";
        if(cursorKey == DF_KEY_LEFT) cursorState = "LEFT ";
        if(cursorKey == DF_KEY_RIGHT) cursorState = "RIGHT";
        lcd.setCursor(6, 1);
        lcd.print(cursorState);
    }

    void currentReading(int reading_) {
        // set the latest reading and trigger the event without notifying, so it waits out the time interval
        encoderReading = reading_;
        setTriggered(true); // we don't want to run the event until the next interval so dont use markAndNotify
    }

    void selectChanged(PaintButtonState state) {
        // set the latest button state and trigger the event without notifying, so it waits out the time interval
        selState = state;
        setTriggered(true); // we don't want to run the event until the next interval so dont use markAndNotify
    }

    void cursor(pinid_t pin) {
        cursorKey = pin;
        setTriggered(true);
    }
} paintEvent;

//
// When you define a 4 joystick button encoder, only two of the buttons are used the for up and down encoder function,
// the other two (next and back) will be provided to you through this callback with whatever pins you defined for next
// and back when you created the up down encoder.
//
class MyPassThroughSwitchListener : public SwitchListener {
public:
    void onPressed(pinid_t pin, bool held) override {
        paintEvent.cursor(pin);
    }

    void onReleased(pinid_t pin, bool held) override {
        paintEvent.cursor(-1);
    }
} passThroughListener;


void setup() {
    // do an initial painting
    paintEvent.markTriggeredAndNotify();

    // set up switches to use the DfRobot input facilities
    switches.init(asIoRef(dfRobotKeys), SWITCHES_POLL_EVERYTHING, false);

    // we setup a rotary encoder on the digital joystick buttons
    setupUpDownButtonEncoder(DF_KEY_UP, DF_KEY_DOWN, DF_KEY_LEFT, DF_KEY_RIGHT, &passThroughListener, [](int reading) {
        paintEvent.currentReading(reading);
    });

    // with a maximum value of 500, starting at 250.
    switches.changeEncoderPrecision(500, 250);

    // now we add a switch handler for the select button
    switches.addSwitch(DF_KEY_SELECT, [](pintype_t , bool held) {
        paintEvent.selectChanged(held ? PaintEvent::BUTTON_HELD : PaintEvent::PRESSED);
    });

    // and we also want to know when it's released. Here we also change the intent letting us cycle through all the
    // intentions that the up down buttons can handle.
    // Change value, regular operation where up and down function normally.
    // Scroll through items, inverts the directions to make moving through lists more natural.
    // Scroll sideways, Uses left/right for encoder value, to make scrolling sideways more natural.
    switches.onRelease(DF_KEY_SELECT, [](pintype_t , bool) {
        paintEvent.selectChanged(PaintEvent::NOT_PRESSED);
        if(intent == CHANGE_VALUE) intent = SCROLL_THROUGH_ITEMS;
        else if(intent == SCROLL_THROUGH_ITEMS) intent = SCROLL_THROUGH_SIDEWAYS;
        else intent = CHANGE_VALUE;
        switches.getEncoder()->setUserIntention(intent);
    });

    // lastly, we set up the event that does the drawing for to a maximum of 4 times a second, when needed.
    taskManager.registerEvent(&paintEvent);
}

void loop() {
    taskManager.runLoop();
}