#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class VintageLook final : public juce::LookAndFeel_V4
{
public:
    VintageLook()
    {
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff463022));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff9a5d34));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff4e3bf));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff4e3bf));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position,
                          float start, float end, juce::Slider&) override
    {
        auto dial = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(3.0f);
        dial = dial.withSizeKeepingCentre(std::min(dial.getWidth(), dial.getHeight()), std::min(dial.getWidth(), dial.getHeight()));
        g.setColour(juce::Colour(0x700e0907));
        g.fillEllipse(dial.translated(0, 3));
        g.setColour(juce::Colour(0xff745942));
        g.fillEllipse(dial);
        dial = dial.reduced(3.0f);
        g.setGradientFill({ juce::Colour(0xfffff2d4), dial.getTopLeft(), juce::Colour(0xffbda27c), dial.getBottomRight(), false });
        g.fillEllipse(dial);
        const auto angle = start + (end - start) * position;
        const auto point = dial.getCentre() + juce::Point<float>(std::sin(angle), -std::cos(angle)) * (dial.getWidth() * .36f);
        g.setColour(juce::Colour(0xff4b3021));
        g.drawLine({ dial.getCentre(), point }, 2.4f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& colour,
                              bool hover, bool down) override
    {
        if (button.getName() == "tuner")
        {
            const auto face = button.getLocalBounds().toFloat().reduced(2);
            g.setColour(juce::Colour(0xff211710));
            g.fillRoundedRectangle(face.translated(0, 2), 9);
            g.setGradientFill({ juce::Colour(hover ? 0xff4b3b2d : 0xff3a2e23), face.getTopLeft(),
                                juce::Colour(0xff1d1611), face.getBottomRight(), false });
            g.fillRoundedRectangle(face, 9);
            g.setColour(juce::Colour(0xff927555));
            g.drawRoundedRectangle(face, 9, button.hasKeyboardFocus(false) ? 2.0f : 1.0f);
            const auto text = static_cast<juce::TextButton&>(button).getButtonText();
            const int lit = text.contains("IN TUNE") ? 0 : text.contains("-") ? -1 : text.contains("+") ? 1 : 2;
            for (int i = -1; i <= 1; ++i)
            {
                g.setColour(juce::Colour(i == lit ? 0xffecc68c : 0xff695748));
                const auto x = face.getCentreX() + static_cast<float>(i) * 19.0f;
                if (i == 0) g.fillRoundedRectangle(x - 6, 9, 12, 4, 2);
                else
                {
                    juce::Path arrow;
                    const auto sign = static_cast<float>(i);
                    arrow.addTriangle(x - sign * 5.0f, 9, x + sign * 4.0f, 11, x - sign * 5.0f, 13);
                    g.fillPath(arrow);
                }
            }
            return;
        }
        if (button.getName() != "footswitch")
        {
            LookAndFeel_V4::drawButtonBackground(g, button, colour, hover, down);
            return;
        }
        auto bounds = button.getLocalBounds().toFloat();
        auto cap = bounds.removeFromTop(50).withSizeKeepingCentre(42, 42).translated(0, down ? 2.0f : 0.0f);
        g.setColour(juce::Colour(0x900e0907));
        g.fillEllipse(cap.translated(0, 3));
        g.setColour(juce::Colour(0xff8e7050));
        g.fillEllipse(cap);
        cap = cap.reduced(4);
        g.setGradientFill({ juce::Colour(hover ? 0xfffff6e5 : 0xffe6d9c1), cap.getTopLeft(),
                            juce::Colour(0xff92806c), cap.getBottomRight(), false });
        g.fillEllipse(cap);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool hover, bool down) override
    {
        if (button.getName() == "tuner")
        {
            const auto bounds = button.getLocalBounds();
            g.setColour(juce::Colour(0xffc7b294));
            g.setFont(juce::FontOptions("Consolas", 8.0f, juce::Font::bold));
            g.drawText("A · B · C · D · E · F · G", bounds.withTrimmedTop(4).withHeight(14), juce::Justification::centred);
            g.setColour(juce::Colour(0xfff1c47e));
            g.fillRoundedRectangle(bounds.getCentreX() - 6.0f, 4.0f, 12.0f, 3.0f, 1.5f);
            g.setColour(juce::Colour(0xfff4e3bf));
            g.setFont(juce::FontOptions("Consolas", 8.0f, juce::Font::bold));
            g.drawFittedText(button.getButtonText(), bounds.withTrimmedTop(18).withHeight(15).reduced(5, 0),
                             juce::Justification::centred, 1);
            g.setColour(juce::Colour(0xffc3ad92));
            g.setFont(juce::FontOptions(6.0f, juce::Font::bold));
            g.drawText("TUNER · DEMO", bounds.withTrimmedTop(34), juce::Justification::centred);
            return;
        }
        if (button.getName() != "footswitch")
        {
            LookAndFeel_V4::drawButtonText(g, button, hover, down);
            return;
        }
        g.setColour(juce::Colour(0xfff4e3bf));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        if (button.hasKeyboardFocus(false))
            g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(1), 5, 1);
        g.drawText(button.getButtonText(), button.getLocalBounds().withTrimmedTop(47), juce::Justification::centred, true);
    }
};

class VintageCard final : public juce::Component
{
public:
    VintageCard(juce::String title, bool isDeck)
        : name(std::move(title)), deck(isDeck)
    {
        setWantsKeyboardFocus(true);
        setTitle(name);
        setDescription("Drag the header to reorder, or press Left or Right.");
    }

    void paint(juce::Graphics& g) override
    {
        auto body = getLocalBounds().toFloat().reduced(3, 5);
        g.setColour(juce::Colour(lifted ? 0xb01e100a : 0x901e100a));
        g.fillRoundedRectangle(body.translated(0, lifted ? 9.0f : 5.0f), 12);
        g.setGradientFill({ juce::Colour(deck ? 0xff795747 : 0xfff5e5d1), body.getTopLeft(),
                            juce::Colour(deck ? 0xff55382d : 0xffc9aa8b), body.getBottomRight(), false });
        g.fillRoundedRectangle(body, 11);
        for (int y = 12; y < getHeight() - 12; y += 3)
        {
            g.setColour(juce::Colour(deck ? (y % 2 == 0 ? 0x17852f17 : 0x15ffe3bc) : 0x120e0905));
            for (int x = 10 + y % 2; x < getWidth() - 10; x += deck ? 4 : 7)
                g.fillRect(x, y, 1, 1);
        }
        g.setColour(juce::Colour(deck ? 0xffb79768 : 0xffa58d70));
        g.drawRoundedRectangle(body.reduced(1), 10, hasKeyboardFocus(false) ? 2.5f : 1.0f);
        if (deck)
        {
            for (auto corner : { body.getTopLeft(), body.getTopRight(), body.getBottomLeft(), body.getBottomRight() })
            {
                auto fitting = juce::Rectangle<float>(22, 22).withCentre(corner + juce::Point<float>(
                    corner.x < body.getCentreX() ? 10.0f : -10.0f, corner.y < body.getCentreY() ? 10.0f : -10.0f));
                g.setGradientFill({ juce::Colour(0xffffecc6), fitting.getTopLeft(), juce::Colour(0xff826343), fitting.getBottomRight(), false });
                g.fillRoundedRectangle(fitting, 4);
            }
            g.setColour(juce::Colour(0xffe5cfa8));
            g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
            g.drawText("01 / LOOP ENGINE", 20, 18, getWidth() / 2, 14, juce::Justification::left);
            g.drawText("STEREO", getWidth() - 82, 18, 60, 14, juce::Justification::right);
            auto plate = juce::Rectangle<float>(180, 40).withCentre({ body.getCentreX(), 48.0f });
            g.setGradientFill({ juce::Colour(0xff8a6844), plate.getTopLeft(), juce::Colour(0xff60482f), plate.getBottomRight(), false });
            g.fillRoundedRectangle(plate, 4);
            g.setColour(juce::Colour(0xffc39e6b));
            g.drawRoundedRectangle(plate, 4, 1.0f);
            g.setColour(juce::Colour(0xfff3ddba));
            g.setFont(juce::FontOptions("Georgia", 24.0f, juce::Font::italic));
            g.drawFittedText(name, plate.toNearestInt().reduced(8, 0), juce::Justification::centred, 1);
        }
        else
        {
            for (auto corner : { body.getTopLeft(), body.getTopRight(), body.getBottomLeft(), body.getBottomRight() })
            {
                g.setColour(juce::Colour(0x66453526));
                g.fillEllipse(juce::Rectangle<float>(6, 6).withCentre(corner + juce::Point<float>(
                    corner.x < body.getCentreX() ? 15.0f : -15.0f, corner.y < body.getCentreY() ? 15.0f : -15.0f)));
            }
            g.setColour(juce::Colour(0xff392a1f));
            g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
            g.drawText("02 / POST-LOOP", 20, 18, getWidth() / 2, 14, juce::Justification::left);
            g.drawText("DEMO", getWidth() - 72, 18, 50, 14, juce::Justification::right);
            g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
            g.drawText(name.toUpperCase(), 18, 36, getWidth() - 36, 34, juce::Justification::centred);
        }
        if (paintFace) paintFace(g);
    }

    juce::MouseCursor getMouseCursor() override
    {
        return juce::MouseCursor(lifted ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }
    void mouseMove(const juce::MouseEvent& event) override
    {
        setMouseCursor(event.y < 65 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }
    void mouseDown(const juce::MouseEvent& event) override
    {
        draggable = event.y < 65 && event.mods.isLeftButtonDown();
        if (draggable) grabKeyboardFocus();
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (draggable && (lifted || event.getDistanceFromDragStart() > 5) && dragMove)
            dragMove(event.getScreenPosition(), event.getMouseDownScreenPosition());
    }
    void mouseUp(const juce::MouseEvent& event) override
    {
        if (lifted && dragEnd) dragEnd(event.getScreenPosition());
        draggable = false;
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (keyMove && (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey))
        {
            keyMove(key == juce::KeyPress::leftKey ? -1 : 1);
            return true;
        }
        if (lifted && key == juce::KeyPress::escapeKey && dragCancel)
        {
            draggable = false;
            dragCancel();
            return true;
        }
        return false;
    }
    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType) override { repaint(); }

    bool lifted = false;
    std::function<void(juce::Graphics&)> paintFace;
    std::function<void(juce::Point<int>, juce::Point<int>)> dragMove;
    std::function<void(juce::Point<int>)> dragEnd;
    std::function<void()> dragCancel;
    std::function<void(int)> keyMove;

private:
    juce::String name;
    bool deck, draggable = false;
};
