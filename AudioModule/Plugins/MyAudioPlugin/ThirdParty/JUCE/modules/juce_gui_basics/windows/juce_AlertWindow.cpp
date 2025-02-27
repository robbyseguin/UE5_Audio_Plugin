/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

static juce_wchar getDefaultPasswordChar() noexcept
{
   #if JUCE_LINUX || JUCE_BSD
    return 0x2022;
   #else
    return 0x25cf;
   #endif
}

static int showAlertWindowUnmanaged (const MessageBoxOptions& opts, ModalComponentManager::Callback* cb)
{
    return detail::ConcreteScopedMessageBoxImpl::showUnmanaged (detail::AlertWindowHelpers::create (opts), cb);
}

//==============================================================================
AlertWindow::AlertWindow (const String& title,
                          const String& message,
                          MessageBoxIconType iconType,
                          Component* comp)
   : TopLevelWindow (title, true),
     alertIconType (iconType),
     associatedComponent (comp),
     desktopScale (comp != nullptr ? Component::getApproximateScaleFactorForComponent (comp) : 1.0f)
{
    setAlwaysOnTop (WindowUtils::areThereAnyAlwaysOnTopWindows());

    accessibleMessageLabel.setColour (Label::textColourId,       Colours::transparentBlack);
    accessibleMessageLabel.setColour (Label::backgroundColourId, Colours::transparentBlack);
    accessibleMessageLabel.setColour (Label::outlineColourId,    Colours::transparentBlack);
    accessibleMessageLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (accessibleMessageLabel);

    if (message.isEmpty())
        text = " "; // to force an update if the message is empty

    setMessage (message);

    AlertWindow::lookAndFeelChanged();
    constrainer.setMinimumOnscreenAmounts (0x10000, 0x10000, 0x10000, 0x10000);
}

AlertWindow::~AlertWindow()
{
    // Ensure that the focus does not jump to another TextEditor while we
    // remove children.
    for (auto* t : textBoxes)
        t->setWantsKeyboardFocus (false);

    // Give away focus before removing the editors, so that any TextEditor
    // with focus has a chance to dismiss native keyboard if shown.
    giveAwayKeyboardFocus();

    removeAllChildren();
}

void AlertWindow::userTriedToCloseWindow()
{
    if (escapeKeyCancels || buttons.size() > 0)
        exitModalState (0);
}

//==============================================================================
void AlertWindow::setMessage (const String& message)
{
    auto newMessage = message.substring (0, 2048);

    if (text != newMessage)
    {
        text = newMessage;

        auto accessibleText = getName() + ". " + text;
        accessibleMessageLabel.setText (accessibleText, NotificationType::dontSendNotification);
        setDescription (accessibleText);

        updateLayout (true);
        repaint();
    }
}

//==============================================================================
void AlertWindow::exitAlert (Button* button)
{
    if (auto* parent = button->getParentComponent())
        parent->exitModalState (button->getCommandID());
}

//==============================================================================
void AlertWindow::addButton (const String& name,
                             const int returnValue,
                             const KeyPress& shortcutKey1,
                             const KeyPress& shortcutKey2)
{
    auto* b = new TextButton (name, {});
    buttons.add (b);

    b->setWantsKeyboardFocus (true);
    b->setExplicitFocusOrder (1);
    b->setMouseClickGrabsKeyboardFocus (false);
    b->setCommandToTrigger (nullptr, returnValue, false);
    b->addShortcut (shortcutKey1);
    b->addShortcut (shortcutKey2);
    b->onClick = [this, b] { exitAlert (b); };

    Array<TextButton*> buttonsArray (buttons.begin(), buttons.size());
    auto& lf = getLookAndFeel();

    auto buttonHeight = lf.getAlertWindowButtonHeight();
    auto buttonWidths = lf.getWidthsForTextButtons (*this, buttonsArray);

    jassert (buttonWidths.size() == buttons.size());
    int i = 0;

    for (auto* button : buttons)
        button->setSize (buttonWidths[i++], buttonHeight);

    addAndMakeVisible (b, 0);
    updateLayout (false);
}

int AlertWindow::getNumButtons() const
{
    return buttons.size();
}

Button* AlertWindow::getButton (int index) const
{
    return buttons[index];
}

Button* AlertWindow::getButton (const String& buttonName) const
{
    for (auto* button : buttons)
        if (buttonName == button->getName())
            return button;

    return nullptr;
}

void AlertWindow::triggerButtonClick (const String& buttonName)
{
    if (auto* button = getButton (buttonName))
        button->triggerClick();
}

void AlertWindow::setEscapeKeyCancels (bool shouldEscapeKeyCancel)
{
    escapeKeyCancels = shouldEscapeKeyCancel;
}

//==============================================================================
void AlertWindow::addTextEditor (const String& name,
                                 const String& initialContents,
                                 const String& onScreenLabel,
                                 const bool isPasswordBox)
{
    auto* ed = new TextEditor (name, isPasswordBox ? getDefaultPasswordChar() : 0);
    ed->setSelectAllWhenFocused (true);
    ed->setEscapeAndReturnKeysConsumed (false);
    textBoxes.add (ed);
    allComps.add (ed);

    ed->setColour (TextEditor::outlineColourId, findColour (ComboBox::outlineColourId));
    ed->setFont (getLookAndFeel().getAlertWindowMessageFont());
    addAndMakeVisible (ed);
    ed->setText (initialContents);
    ed->setCaretPosition (initialContents.length());
    textboxNames.add (onScreenLabel);

    updateLayout (false);
}

TextEditor* AlertWindow::getTextEditor (const String& nameOfTextEditor) const
{
    for (auto* tb : textBoxes)
        if (tb->getName() == nameOfTextEditor)
            return tb;

    return nullptr;
}

String AlertWindow::getTextEditorContents (const String& nameOfTextEditor) const
{
    if (auto* t = getTextEditor (nameOfTextEditor))
        return t->getText();

    return {};
}


//==============================================================================
void AlertWindow::addComboBox (const String& name,
                               const StringArray& items,
                               const String& onScreenLabel)
{
    auto* cb = new ComboBox (name);
    comboBoxes.add (cb);
    allComps.add (cb);

    cb->addItemList (items, 1);

    addAndMakeVisible (cb);
    cb->setSelectedItemIndex (0);

    comboBoxNames.add (onScreenLabel);
    updateLayout (false);
}

ComboBox* AlertWindow::getComboBoxComponent (const String& nameOfList) const
{
    for (auto* cb : comboBoxes)
        if (cb->getName() == nameOfList)
            return cb;

    return nullptr;
}

//==============================================================================
class AlertTextComp final : public TextEditor
{
public:
    AlertTextComp (AlertWindow& owner, const String& message, const Font& font)
    {
        if (owner.isColourSpecified (AlertWindow::textColourId))
            setColour (TextEditor::textColourId, owner.findColour (AlertWindow::textColourId));

        setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
        setColour (TextEditor::outlineColourId, Colours::transparentBlack);
        setColour (TextEditor::shadowColourId, Colours::transparentBlack);

        setReadOnly (true);
        setMultiLine (true, true);
        setCaretVisible (false);
        setScrollbarsShown (true);
        lookAndFeelChanged();
        setWantsKeyboardFocus (false);
        setFont (font);
        setText (message, false);

        bestWidth = 2 * (int) std::sqrt (font.getHeight() * GlyphArrangement::getStringWidth (font, message));
    }

    void updateLayout (const int width)
    {
        AttributedString s;
        s.setJustification (Justification::topLeft);
        s.append (getText(), getFont());

        TextLayout text;
        text.createLayoutWithBalancedLineLengths (s, (float) width - 8.0f);
        setSize (width, jmin (width, (int) (text.getHeight() + getFont().getHeight())));
    }

    int bestWidth;

    JUCE_DECLARE_NON_COPYABLE (AlertTextComp)
};

void AlertWindow::addTextBlock (const String& textBlock)
{
    auto* c = new AlertTextComp (*this, textBlock, getLookAndFeel().getAlertWindowMessageFont());
    textBlocks.add (c);
    allComps.add (c);
    addAndMakeVisible (c);

    updateLayout (false);
}

//==============================================================================
void AlertWindow::addProgressBarComponent (double& progressValue, std::optional<ProgressBar::Style> style)
{
    auto* pb = new ProgressBar (progressValue, style);
    progressBars.add (pb);
    allComps.add (pb);
    addAndMakeVisible (pb);

    updateLayout (false);
}

//==============================================================================
void AlertWindow::addCustomComponent (Component* const component)
{
    customComps.add (component);
    allComps.add (component);
    addAndMakeVisible (component);

    updateLayout (false);
}

int AlertWindow::getNumCustomComponents() const                 { return customComps.size(); }
Component* AlertWindow::getCustomComponent (int index) const    { return customComps [index]; }

Component* AlertWindow::removeCustomComponent (const int index)
{
    auto* c = getCustomComponent (index);

    if (c != nullptr)
    {
        customComps.removeFirstMatchingValue (c);
        allComps.removeFirstMatchingValue (c);
        removeChildComponent (c);

        updateLayout (false);
    }

    return c;
}

//==============================================================================
void AlertWindow::paint (Graphics& g)
{
    auto& lf = getLookAndFeel();
    lf.drawAlertBox (g, *this, textArea, textLayout);

    g.setColour (findColour (textColourId));
    g.setFont (lf.getAlertWindowFont());

    for (int i = textBoxes.size(); --i >= 0;)
    {
        auto* te = textBoxes.getUnchecked (i);

        g.drawFittedText (textboxNames[i],
                          te->getX(), te->getY() - 14,
                          te->getWidth(), 14,
                          Justification::centredLeft, 1);
    }

    for (int i = comboBoxNames.size(); --i >= 0;)
    {
        auto* cb = comboBoxes.getUnchecked (i);

        g.drawFittedText (comboBoxNames[i],
                          cb->getX(), cb->getY() - 14,
                          cb->getWidth(), 14,
                          Justification::centredLeft, 1);
    }

    for (auto* c : customComps)
        g.drawFittedText (c->getName(),
                          c->getX(), c->getY() - 14,
                          c->getWidth(), 14,
                          Justification::centredLeft, 1);
}

void AlertWindow::updateLayout (const bool onlyIncreaseSize)
{
    const int titleH = 24;
    const int iconWidth = 80;

    auto& lf = getLookAndFeel();
    auto messageFont (lf.getAlertWindowMessageFont());

    auto wid = jmax (GlyphArrangement::getStringWidth (messageFont, text),
                     GlyphArrangement::getStringWidth (messageFont, getName()));

    auto sw = (int) std::sqrt (messageFont.getHeight() * (float) wid);
    auto w = jmin (300 + sw * 2, (int) ((float) getParentWidth() * 0.7f));
    const int edgeGap = 10;
    const int labelHeight = 18;
    int iconSpace = 0;

    AttributedString attributedText;
    attributedText.append (getName(), lf.getAlertWindowTitleFont());

    if (text.isNotEmpty())
        attributedText.append ("\n\n" + text, messageFont);

    attributedText.setColour (findColour (textColourId));

    if (alertIconType == NoIcon)
    {
        attributedText.setJustification (Justification::centredTop);
        textLayout.createLayoutWithBalancedLineLengths (attributedText, (float) w);
    }
    else
    {
        attributedText.setJustification (Justification::topLeft);
        textLayout.createLayoutWithBalancedLineLengths (attributedText, (float) w);
        iconSpace = iconWidth;
    }

    w = jmax (350, (int) textLayout.getWidth() + iconSpace + edgeGap * 4);
    w = jmin (w, (int) ((float) getParentWidth() * 0.7f));

    auto textLayoutH = (int) textLayout.getHeight();
    auto textBottom = 16 + titleH + textLayoutH;
    int h = textBottom;

    int buttonW = 40;

    for (auto* b : buttons)
        buttonW += 16 + b->getWidth();

    w = jmax (buttonW, w);

    h += (textBoxes.size() + comboBoxes.size() + progressBars.size()) * 50;

    if (auto* b = buttons[0])
        h += 20 + b->getHeight();

    for (auto* c : customComps)
    {
        w = jmax (w, (c->getWidth() * 100) / 80);
        h += 10 + c->getHeight();

        if (c->getName().isNotEmpty())
            h += labelHeight;
    }

    for (auto* tb : textBlocks)
        w = jmax (w, static_cast<const AlertTextComp*> (tb)->bestWidth);

    w = jmin (w, (int) ((float) getParentWidth() * 0.7f));

    for (auto* tb : textBlocks)
    {
        auto* ac = static_cast<AlertTextComp*> (tb);
        ac->updateLayout ((int) ((float) w * 0.8f));
        h += ac->getHeight() + 10;
    }

    h = jmin (getParentHeight() - 50, h);

    if (onlyIncreaseSize)
    {
        w = jmax (w, getWidth());
        h = jmax (h, getHeight());
    }

    if (! isVisible())
        centreAroundComponent (associatedComponent, w, h);
    else
        setBounds (getBounds().withSizeKeepingCentre (w, h));

    textArea.setBounds (edgeGap, edgeGap, w - (edgeGap * 2), h - edgeGap);
    accessibleMessageLabel.setBounds (textArea);

    const int spacer = 16;
    int totalWidth = -spacer;

    for (auto* b : buttons)
        totalWidth += b->getWidth() + spacer;

    auto x = (w - totalWidth) / 2;
    auto y = (int) ((float) getHeight() * 0.95f);

    for (auto* c : buttons)
    {
        int ny = proportionOfHeight (0.95f) - c->getHeight();
        c->setTopLeftPosition (x, ny);
        if (ny < y)
            y = ny;

        x += c->getWidth() + spacer;

        c->toFront (false);
    }

    y = textBottom;

    for (auto* c : allComps)
    {
        h = 22;

        const int comboIndex = comboBoxes.indexOf (dynamic_cast<ComboBox*> (c));
        if (comboIndex >= 0 && comboBoxNames [comboIndex].isNotEmpty())
            y += labelHeight;

        const int tbIndex = textBoxes.indexOf (dynamic_cast<TextEditor*> (c));
        if (tbIndex >= 0 && textboxNames[tbIndex].isNotEmpty())
            y += labelHeight;

        if (customComps.contains (c))
        {
            if (c->getName().isNotEmpty())
                y += labelHeight;

            c->setTopLeftPosition (proportionOfWidth (0.1f), y);
            h = c->getHeight();
        }
        else if (textBlocks.contains (c))
        {
            c->setTopLeftPosition ((getWidth() - c->getWidth()) / 2, y);
            h = c->getHeight();
        }
        else
        {
            c->setBounds (proportionOfWidth (0.1f), y, proportionOfWidth (0.8f), h);
        }

        y += h + 10;
    }

    setWantsKeyboardFocus (getNumChildComponents() == 0);
}

bool AlertWindow::containsAnyExtraComponents() const
{
    return allComps.size() > 0;
}

//==============================================================================
void AlertWindow::mouseDown (const MouseEvent& e)
{
    dragger.startDraggingComponent (this, e);
}

void AlertWindow::mouseDrag (const MouseEvent& e)
{
    dragger.dragComponent (this, e, &constrainer);
}

bool AlertWindow::keyPressed (const KeyPress& key)
{
    for (auto* b : buttons)
    {
        if (b->isRegisteredForShortcut (key))
        {
            b->triggerClick();
            return true;
        }
    }

    if (key.isKeyCode (KeyPress::escapeKey) && escapeKeyCancels)
    {
        exitModalState (0);
        return true;
    }

    if (key.isKeyCode (KeyPress::returnKey) && buttons.size() == 1)
    {
        buttons.getUnchecked (0)->triggerClick();
        return true;
    }

    return false;
}

void AlertWindow::lookAndFeelChanged()
{
    const int newFlags = getLookAndFeel().getAlertBoxWindowFlags();

    setUsingNativeTitleBar ((newFlags & ComponentPeer::windowHasTitleBar) != 0);
    setDropShadowEnabled (isOpaque() && (newFlags & ComponentPeer::windowHasDropShadow) != 0);
    updateLayout (false);
}

int AlertWindow::getDesktopWindowStyleFlags() const
{
    return getLookAndFeel().getAlertBoxWindowFlags();
}

//==============================================================================
#if JUCE_MODAL_LOOPS_PERMITTED
void AlertWindow::showMessageBox (MessageBoxIconType iconType,
                                  const String& title,
                                  const String& message,
                                  const String& buttonText,
                                  Component* associatedComponent)
{
    show (MessageBoxOptions()
            .withIconType (iconType)
            .withTitle (title)
            .withMessage (message)
            .withButton (buttonText.isEmpty() ? TRANS ("OK") : buttonText)
            .withAssociatedComponent (associatedComponent));
}

int AlertWindow::show (const MessageBoxOptions& options)
{
    if (LookAndFeel::getDefaultLookAndFeel().isUsingNativeAlertWindows())
        return NativeMessageBox::show (options);

    return showAlertWindowUnmanaged (options, nullptr);
}

bool AlertWindow::showNativeDialogBox (const String& title,
                                       const String& bodyText,
                                       bool isOkCancel)
{
    if (isOkCancel)
        return NativeMessageBox::showOkCancelBox (AlertWindow::NoIcon, title, bodyText);

    NativeMessageBox::showMessageBox (AlertWindow::NoIcon, title, bodyText);
    return true;
}
#endif

void AlertWindow::showAsync (const MessageBoxOptions& options, ModalComponentManager::Callback* callback)
{
    if (LookAndFeel::getDefaultLookAndFeel().isUsingNativeAlertWindows())
        NativeMessageBox::showAsync (options, callback);
    else
        showAlertWindowUnmanaged (options, callback);
}

void AlertWindow::showAsync (const MessageBoxOptions& options, std::function<void (int)> callback)
{
    showAsync (options, ModalCallbackFunction::create (callback));
}

void AlertWindow::showMessageBoxAsync (MessageBoxIconType iconType,
                                       const String& title,
                                       const String& message,
                                       const String& buttonText,
                                       Component* associatedComponent,
                                       ModalComponentManager::Callback* callback)
{
    auto options = MessageBoxOptions::makeOptionsOk (iconType,
                                                     title,
                                                     message,
                                                     buttonText,
                                                     associatedComponent);
    showAsync (options, callback);
}

static int showMaybeAsync (const MessageBoxOptions& options,
                           ModalComponentManager::Callback* callbackIn)
{
    if (LookAndFeel::getDefaultLookAndFeel().isUsingNativeAlertWindows())
        return showNativeBoxUnmanaged (options, callbackIn, ResultCodeMappingMode::alertWindow);

    return showAlertWindowUnmanaged (options, callbackIn);
}

bool AlertWindow::showOkCancelBox (MessageBoxIconType iconType,
                                   const String& title,
                                   const String& message,
                                   const String& button1Text,
                                   const String& button2Text,
                                   Component* associatedComponent,
                                   ModalComponentManager::Callback* callback)
{
    auto options = MessageBoxOptions::makeOptionsOkCancel (iconType,
                                                           title,
                                                           message,
                                                           button1Text,
                                                           button2Text,
                                                           associatedComponent);
    return showMaybeAsync (options, callback) == 1;
}

int AlertWindow::showYesNoCancelBox (MessageBoxIconType iconType,
                                     const String& title,
                                     const String& message,
                                     const String& button1Text,
                                     const String& button2Text,
                                     const String& button3Text,
                                     Component* associatedComponent,
                                     ModalComponentManager::Callback* callback)
{
    auto options = MessageBoxOptions::makeOptionsYesNoCancel (iconType,
                                                              title,
                                                              message,
                                                              button1Text,
                                                              button2Text,
                                                              button3Text,
                                                              associatedComponent);
    return showMaybeAsync (options, callback);
}

ScopedMessageBox AlertWindow::showScopedAsync (const MessageBoxOptions& options, std::function<void (int)> callback)
{
    if (LookAndFeel::getDefaultLookAndFeel().isUsingNativeAlertWindows())
        return NativeMessageBox::showScopedAsync (options, std::move (callback));

    return detail::ConcreteScopedMessageBoxImpl::show (detail::AlertWindowHelpers::create (options), std::move (callback));
}

//==============================================================================
std::unique_ptr<AccessibilityHandler> AlertWindow::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::dialogWindow);
}

} // namespace juce
