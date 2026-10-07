#include "Clickable.h"

#include "DragScroller.h"
#include "core/Help.h"

namespace t3k::ui {

namespace {

juce::AccessibilityActions buttonActions(Clickable& button) {
  juce::AccessibilityActions actions;
  actions.addAction(juce::AccessibilityActionType::press, [&button] {
    if (button.isShowing()) button.grabKeyboardFocus();
    button.triggerClick();
  });
  if (button.isToggleable())
    actions.addAction(juce::AccessibilityActionType::toggle, [&button] {
      if (button.isShowing()) button.grabKeyboardFocus();
      button.triggerClick();
    });
  return actions;
}

// juce::Button's own handler is not public; this is the part of it we use
// (a pressable button, checked when toggleable) with our naming.
class Handler : public juce::AccessibilityHandler {
public:
  explicit Handler(Clickable& button)
      : juce::AccessibilityHandler(button, button.isToggleable() ? juce::AccessibilityRole::toggleButton
                                                                : juce::AccessibilityRole::button,
                                   buttonActions(button)),
        button_(button) {}

  juce::String getTitle() const override { return button_.accessibleName(); }
  juce::String getHelp() const override { return button_.getHelpText(); }
  juce::AccessibleState getCurrentState() const override {
    auto state = juce::AccessibilityHandler::getCurrentState();
    if (!button_.isToggleable()) return state;
    return button_.getToggleState() ? state.withCheckable().withChecked() : state.withCheckable();
  }

private:
  Clickable& button_;
};

}  // namespace

Clickable::Clickable(const juce::String& text) : juce::Button(text) {
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
}

juce::String Clickable::accessibleName() const {
  if (getTitle().isNotEmpty()) return getTitle();
  if (getButtonText().isNotEmpty()) return getButtonText();
  if (getName().isNotEmpty()) return getName();  // owners name text-less buttons for the testbed
  return help::lead(getHelpText());
}

std::unique_ptr<juce::AccessibilityHandler> Clickable::createAccessibilityHandler() {
  return std::make_unique<Handler>(*this);
}

// The button's own handler runs before the viewport's listeners, so on the
// release the pan is still in progress here.
void Clickable::mouseDrag(const juce::MouseEvent& e) {
  if (DragScroller::panning(*this)) setState(buttonNormal);
  else juce::Button::mouseDrag(e);
}

void Clickable::mouseUp(const juce::MouseEvent& e) {
  if (DragScroller::panning(*this)) setState(buttonNormal);
  else juce::Button::mouseUp(e);
}

}  // namespace t3k::ui
