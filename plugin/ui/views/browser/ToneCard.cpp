#include "ToneCard.h"

#include <algorithm>
#include <cmath>

#include "core/Brand.h"
#include "core/Design.h"
#include "core/Fonts.h"
#include "core/Icons.h"
#include "core/Labels.h"
#include "core/Paint.h"
#include "core/Theme.h"
#include "model/ToneOverview.h"
#include "views/block/BlockInfoPanel.h"
#include "widgets/DragScroller.h"
#include "widgets/SecondaryPress.h"

namespace t3k::ui {

namespace {
const juce::String kDotSeparator = juce::String::fromUTF8(" \xC2\xB7 ");
// The verified badge beside the creator's name: 14px tall at its 17.5:20.
constexpr float kBadgeHeight = 14;
constexpr float kBadgeWidth = kBadgeHeight * 17.5f / 20.0f;
constexpr float kBadgeGap = 6;
}  // namespace

class ToneCard::InfoPopover : public Popover {
public:
  InfoPopover(const Tone& tone, const juce::String& url, std::function<void(const juce::String&)> openUrl)
      : scroller_(DragScroller::Axis::vertical, DragScroller::Keys::none), close_("Close", PillButton::Style::outline) {
    setTitle(tone.title + " — More info");
    title_.setText(tone.title, juce::dontSendNotification);
    title_.setFont(Fonts::sans(16, true));
    title_.setColour(juce::Label::textColourId, theme::kWhite);
    addAndMakeVisible(title_);
    close_.setHelpText("Close tone information and return to the browsing result.");
    close_.onClick = [this] { dismiss(); };
    addAndMakeVisible(close_);
    scroller_.setViewedComponent(&panel_, false);
    addAndMakeVisible(scroller_);
    BlockInfoPanel::State state;
    // This payload was already returned by the accessible catalog feed.
    // Inspecting it never requests a model or modifies the audio chain.
    state.authenticated = true;
    state.tone = tone;
    state.pageUrl = url;
    panel_.setState(std::move(state));
    panel_.onHeightChanged = [this] { resized(); };
    panel_.onOpenUrl = std::move(openUrl);
    setSize(480, 420);
  }
  void resized() override {
    auto area = contentBounds().reduced(16);
    auto header = area.removeFromTop(36);
    close_.setBounds(header.removeFromRight(76));
    title_.setBounds(header.reduced(0, 2));
    area.removeFromTop(12);
    scroller_.setBounds(area);
    const int width = juce::jmax(1, area.getWidth() - 12);
    panel_.setSize(width, panel_.heightFor(width));
  }
  void paint(juce::Graphics& g) override {
    const auto box = getLocalBounds().toFloat();
    paint::fill(g, box, theme::kPanelCorner, theme::kPanelBg);
    paint::border(g, box, theme::kPanelCorner, theme::kBorder);
  }
private:
  juce::Label title_;
  BlockInfoPanel panel_;
  DragScroller scroller_;
  PillButton close_;
};

namespace {
class ToneCardAccessibility : public juce::AccessibilityHandler {
public:
  explicit ToneCardAccessibility(ToneCard& card)
      : juce::AccessibilityHandler(card, juce::AccessibilityRole::button,
            juce::AccessibilityActions()
                .addAction(juce::AccessibilityActionType::press, [&card] {
                  if (!card.canSelect()) return;
                  if (card.isShowing()) card.grabKeyboardFocus();
                  card.triggerClick();
                })
                .addAction(juce::AccessibilityActionType::showMenu, [&card] {
                  if (card.isShowing()) card.grabKeyboardFocus();
                  card.openMenu(card.getLocalBounds().getCentre());
                })), card_(card) {}
  juce::String getTitle() const override { return card_.accessibleName(); }
  juce::String getHelp() const override { return card_.getHelpText(); }
private:
  ToneCard& card_;
};
}  // namespace

ToneCard::ToneCard(ImageLoader& images, const Tone& tone)
    : Clickable(tone.title), tone_(tone), image_(images) {
  onOpenUrl = [](const juce::String& target) { juce::URL(target).launchInDefaultBrowser(); };

  juce::String explanation = tone_.description.trim();
  if (explanation.isEmpty()) {
    for (const auto& gear : labels::gearFilters())
      if (tone_.gear == gear.id) { explanation = gear.description; break; }
    if (explanation.isEmpty()) explanation = "No description supplied by the creator.";
  }
  setHelpText(explanation + " Enter or Space: select this tone. Shift+F10 or Applications key: More info and Open on web.");

  image_.setCornerRadius(kImageCorner);
  image_.setTone(tone_.images.empty() ? juce::String() : tone_.images.front(), tone_.gear, /*local=*/false);
  addAndMakeVisible(image_);

  // The A2 mark only where the plugin can actually load the tone; NAM cards
  // without A2 models render disabled and unmarked.
  badge_.setFormat(labels::format(tone_.format), tone_.isNam() && !unavailable(tone_));
  addAndMakeVisible(badge_);

  addChildComponent(avatar_);
  if (tone_.user) {
    avatar_.setVisible(true);
    avatar_.setImage(images, tone_.user->avatarUrl);
  }
  syncState();
}

ToneCard::~ToneCard() {
  if (menu_) menu_->close();
  if (info_) info_->close();
}

juce::String ToneCard::webUrl() const {
  if (tone_.url.startsWithIgnoreCase("https://") || tone_.url.startsWithIgnoreCase("http://")) return tone_.url;
  return "https://www.tone3000.com/tones/" + juce::String(tone_.id);
}

void ToneCard::openMenu(juce::Point<int> point) {
  if (menu_ && menu_->isOpen()) return;
  // A dismissed row may still be executing its callback.
  if (auto* old = menu_.release()) juce::MessageManager::callAsync([old] { delete old; });
  menu_ = std::make_unique<ContextMenu>(std::vector<ContextMenu::Item>{
      {"More info", Icon::Info, help::Key::toneInfo, [this] { showInfo(); }},
      {"Open on web", Icon::ExternalLink, help::Key::viewOnT3k,
       [this] { if (onOpenUrl) onOpenUrl(webUrl()); }},
  });
  menu_->setTitle(tone_.title + " menu");
  menu_->openAtPoint(*this, point);
}

void ToneCard::showInfo() {
  if (info_) info_->close();
  info_ = std::make_unique<InfoPopover>(tone_, webUrl(), onOpenUrl);
  info_->openAt(*this, getLocalBounds().getCentre());
}

void ToneCard::mouseDown(const juce::MouseEvent& event) {
  contextPress_ = event.mods.isPopupMenu();
  if (contextPress_) { openMenu(event.getPosition()); return; }
  if (menu_ && menu_->isOpen()) {
    menu_->dismiss();
    contextPress_ = true;
    return;
  }
  if (disabled_) return;
  Clickable::mouseDown(event);
}

void ToneCard::mouseUp(const juce::MouseEvent& event) {
  if (contextPress_) { contextPress_ = false; return; }
  if (!disabled_) Clickable::mouseUp(event);
}

bool ToneCard::keyPressed(const juce::KeyPress& key) {
  if ((key.isKeyCode(juce::KeyPress::F10Key) && key.getModifiers().isShiftDown()) ||
      key.isKeyCode(kWindowsApplicationsKey)) {
    openMenu(getLocalBounds().getCentre());
    return true;
  }
  if (disabled_ && (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)) return true;
  return Clickable::keyPressed(key);
}

std::unique_ptr<juce::AccessibilityHandler> ToneCard::createAccessibilityHandler() {
  return std::make_unique<ToneCardAccessibility>(*this);
}

void ToneCard::setLoading(bool loading) {
  if (loading == loading_) return;
  loading_ = loading;
  if (loading_) {
    busy_ = std::make_unique<BusyOverlay>(BusyOverlay::Align::centre);
    busy_->setCornerRadius(kCorner);
    addAndMakeVisible(*busy_);
    busy_->setBounds(getLocalBounds());
  } else {
    busy_.reset();
  }
  syncState();
}

void ToneCard::setDisabled(bool disabled) {
  if (disabled == disabled_) return;
  disabled_ = disabled;
  syncState();
}

void ToneCard::syncState() {
  // Inspection stays available even when the tone cannot currently be loaded.
  setEnabled(true);
  // JUCE has no not-allowed cursor; the dimmed card carries the meaning.
  setMouseCursor(disabled_ ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);
  // opacity: DISABLED_OPACITY while disabled and not the card being picked.
  setAlpha(disabled_ && !loading_ ? theme::kDisabledOpacity : 1.0f);
  repaint();
}

float ToneCard::contentHeightFor(int width) {
  measure(width);
  return 2 * kPad + std::max(static_cast<float>(kImage), columnHeight_);
}

void ToneCard::setContentHeight(float height) {
  contentHeight_ = height;
  resized();
}

void ToneCard::measure(int width) {
  if (width == builtWidth_) return;
  builtWidth_ = width;
  const float colW = static_cast<float>(width - 2 * kPad - kImage - kGapX);
  title_ = std::make_unique<TextFlow>(Fonts::sans(kTitlePx, /*bold=*/true), kTitleLine, tone_.title, colW);
  const float titleH = title_->clampedHeight(kTitleLines);
  const float gearH = static_cast<float>(std::max(Fonts::normalLineHeight(kBodyPx), badge_.isVisible() ? badge_.getHeight() : 0));
  const float statsH = static_cast<float>(std::max(Fonts::normalLineHeight(kBodyPx), kStatIcon));
  columnHeight_ = titleH + kRowGap + gearH + kRowGap + statsH + (tone_.user ? kRowGap + kAvatar : 0);
}

// The grid stretches every card in a row to the tallest; the image and the
// text column each centre in that (fractional) height, align-items: center.
void ToneCard::resized() {
  measure(getWidth());
  const float x = kPad + kImage + kGapX;
  const float colW = static_cast<float>(getWidth() - 2 * kPad - kImage - kGapX);
  const float rowH = (contentHeight_ > 0 ? contentHeight_ : static_cast<float>(getHeight())) - 2 * kPad;
  image_.setBounds(kPad, kPad + design::snap((rowH - kImage) / 2), kImage, kImage);

  float y = kPad + (rowH - columnHeight_) / 2;
  const float titleH = title_->clampedHeight(kTitleLines);
  const float gearH = static_cast<float>(std::max(Fonts::normalLineHeight(kBodyPx), badge_.isVisible() ? badge_.getHeight() : 0));
  const float statsH = static_cast<float>(std::max(Fonts::normalLineHeight(kBodyPx), kStatIcon));
  titleBox_ = {x, y, colW, titleH};
  y += titleH + kRowGap;
  gearRow_ = {x, y, colW, gearH};
  y += gearH + kRowGap;
  statsRow_ = {x, y, colW, statsH};
  y += statsH;
  creatorRow_ = tone_.user ? juce::Rectangle<float>(x, y + kRowGap, colW, static_cast<float>(kAvatar)) : juce::Rectangle<float>();

  // Gear label shrinks first; the badge keeps its width (flex-shrink 0).
  const auto gearLabel = labels::gear(tone_.gear);
  const int badgeW = badge_.isVisible() ? badge_.getWidth() : 0;
  const float labelW = gearLabel.isEmpty() ? 0.0f : Fonts::width(Fonts::sans(kBodyPx), gearLabel);
  const float labelMax = gearRow_.getWidth() - (badgeW > 0 ? badgeW + kGearGap : 0);
  const float badgeX = gearRow_.getX() + (gearLabel.isEmpty() ? 0.0f : std::min(labelW, labelMax) + kGearGap);
  badge_.setTopLeftPosition(design::snap(badgeX),
                            design::snap(gearRow_.getY() + (gearRow_.getHeight() - badge_.getHeight()) / 2.0f));

  if (tone_.user) avatar_.setBounds(juce::roundToInt(creatorRow_.getX()), juce::roundToInt(creatorRow_.getY()), kAvatar, kAvatar);
  if (busy_) busy_->setBounds(getLocalBounds());
}

void ToneCard::paintButton(juce::Graphics& g, bool, bool) {
  paint::fill(g, getLocalBounds().toFloat(), kCorner, theme::kSurface);
  // The image box's raised backdrop shows through until artwork paints.
  paint::fill(g, image_.getBounds().toFloat(), kImageCorner, theme::kSurfaceRaised);

  if (title_) title_->draw(g, titleBox_.getTopLeft(), theme::kWhite, kTitleLines, /*ellipsis=*/true);

  const auto body = Fonts::sans(kBodyPx);
  const float line = static_cast<float>(Fonts::normalLineHeight(kBodyPx));

  // Gear label, ellipsised beside the badge.
  const auto gearLabel = labels::gear(tone_.gear);
  if (gearLabel.isNotEmpty()) {
    const int badgeW = badge_.isVisible() ? badge_.getWidth() : 0;
    const float labelMax = gearRow_.getWidth() - (badgeW > 0 ? badgeW + kGearGap : 0);
    paint::cssLine(g, gearLabel, gearRow_.getX(), gearRow_.getY() + (gearRow_.getHeight() - line) / 2, line, labelMax, body,
                   theme::kMuted);
  }

  // Counts (CountStat): 14px glyph, 6px gap, 13px number, all MUTED.
  {
    float x = statsRow_.getX();
    const float cy = statsRow_.getCentreY();
    const float textTop = statsRow_.getY() + (statsRow_.getHeight() - line) / 2;
    auto stat = [&](Icon icon, const juce::String& count) {
      Icons::draw(g, icon, juce::Rectangle<float>(kStatIcon, kStatIcon).withCentre({x + kStatIcon / 2.0f, cy}), theme::kMuted);
      x += kStatIconWidth;
      x += paint::cssLine(g, count, x, textTop, line, 200, body, theme::kMuted) + kStatsGap;
    };
    stat(Icon::Download, labels::count(tone_.downloadsCount));
    stat(Icon::FolderClosed, labels::count(tone_.catalogModelCount()));
  }

  // Creator: avatar, name, the verified badge for verified creators, and the
  // time ago (dot-separated when there is no badge between), one line: the
  // name ellipsises first, the rest keeps its width.
  if (tone_.user) {
    const auto& user = *tone_.user;
    const auto ago = labels::timeAgoShort(tone_.publishedAt);
    const float lineTop = creatorRow_.getY() + (creatorRow_.getHeight() - line) / 2;
    float x = creatorRow_.getX() + kAvatar + kCreatorGap;
    const float agoW = ago.isNotEmpty() ? Fonts::width(body, ago) : 0.0f;
    const float badgeW = user.isVerified ? kBadgeGap + kBadgeWidth : 0.0f;
    const float sepW = !user.isVerified && ago.isNotEmpty() ? Fonts::width(body, kDotSeparator) : 0.0f;
    const float nameMax = creatorRow_.getRight() - x - badgeW - sepW - agoW - (ago.isNotEmpty() ? kBadgeGap : 0.0f);
    x += paint::cssLine(g, user.name(), x, lineTop, line, std::max(0.0f, nameMax), body, theme::kMuted);
    if (user.isVerified) {
      x += kBadgeGap;
      Brand::drawVerifiedBadge(g, juce::Rectangle<float>(kBadgeWidth, kBadgeHeight).withCentre({x + kBadgeWidth / 2, creatorRow_.getCentreY()}));
      x += kBadgeWidth;
    }
    if (ago.isNotEmpty()) {
      if (!user.isVerified) x += paint::cssLine(g, kDotSeparator, x, lineTop, line, sepW + 1, body, theme::kMuted);
      else x += kBadgeGap;
      paint::cssLine(g, ago, x, lineTop, line, agoW + 1, body, theme::kMuted);
    }
  }
}

}  // namespace t3k::ui
