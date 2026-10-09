# Manual accessibility checks

Automated accessibility regressions are in `SelfTests.cpp` alongside the
other UI self-tests. They check names, roles, keyboard actions, focus
containment and restoration. The dialog suite includes activating an editor
with a dialog already showing. These checks do not establish what NVDA
actually speaks or reproduce REAPER's F6 handoff.

## Dialogs in the installed standalone app and REAPER

From the repository root on Windows, close the application being tested,
then run one of:

```powershell
./script/test-dialogs.ps1 -App Standalone -DialogOnOpen
./script/test-dialogs.ps1 -App REAPER -DialogOnOpen
```

The launcher enables previews only for the new process. Installed binaries
must contain the preview support. The sample update dialog opens each time
a plugin editor is created; its version is `0.0.0-test`, and it cannot
download an update or save reminder choices.

1. In REAPER, add TONE3000 to a track and open its editor. The sample dialog
   appears automatically, before the usual F6 focus handoff.
2. Press F6 to enter the plugin window. Check that NVDA reads the update
   title, version and explanation. In standalone, check the initial speech
   when the window receives focus.
3. Check Tab and Shift+Tab stay within the dialog, Enter activates the
   focused control, and Escape closes it. Check focus returns to an
   appropriate plugin control.
4. Switch away and back while the dialog is open and check focus remains
   usable. If F6 is silent, record what one Tab and NVDA+Tab announce.

For dialogs opened after entering the editor, omit `-DialogOnOpen` and use:

| Shortcut | Sample dialog |
| --- | --- |
| Ctrl+Alt+Shift+U | Update |
| Ctrl+Alt+Shift+O | Offline |
| Ctrl+Alt+Shift+S | Secure connection |

Normal application launches do not enable these previews.

## Controls and tone loading in REAPER

Use a normal REAPER launch, enter the TONE3000 editor with F6, then:

1. Click a power button and adjust a knob by mouse and by NVDA's value
   controls. NVDA+Tab should report that control; subsequent Tab and arrow
   keys should work without another F6.
2. Open an option menu, then dismiss it. Focus should return to its button.
3. Activate an empty slot. Focus should move into Select Tone. Activate
   Amp Head or Spaces: its checked state and the loading announcement should
   change, then the result count or empty/error message should be spoken.
4. Read each gear filter's accessible help, also shown in the app's hint
   bar when hints are enabled. These controls filter the list; choosing a
   result loads a tone. Pressing a selected filter again clears it.
5. While signed out, choosing a result opens the sign-in prompt. Its button
   exposes the explanation through accessible help and description.

## Toggles and Advanced panels

1. Toggle power, PRE, STEP, Solo and polarity controls. NVDA should report
   their checked state, and NVDA+Tab should identify the control just used.
2. Enable and disable Spread or Align. Focus should move between the
   enable pill and the matching power button when the layout changes,
   with the feature's on/off state announced.
3. Focus Gate, Pitch or the Spread/Align controls and press Shift+F10 or
   the Windows Applications key. The Advanced panel should open. Tab or
   arrows enter its controls; Escape returns to the exact opening control.
4. Repeat with the physical Applications key in REAPER. Automated tests
   verify context-action routing, but the native key-state delivery and
   NVDA's actual speech require this manual pass.

## Manual result

On 2026-10-06, the user confirmed the dialog-on-open test worked after the
announcement was deferred until dialog focus. Repeat the manual test after
changes to dialogs, editor activation or accessibility announcements.


## Menu, page and tuner keyboard checks

- Context menus and advanced panels: Up/Down moves between controls; Left/Right adjusts a focused slider. Tab/Shift+Tab also moves between controls. In text entry fields use Tab/Shift+Tab so arrow keys remain available for editing. Enter activates or edits; Escape closes and restores focus.
- Tone-results Pages: Up/Right advances, Down/Left goes back, Home/End selects first/last. Verify the accessible page value changes.
- Mono/Stereo: activation announces the selected mode and exposes checked state.
- Tone results: the original creator description appears in accessible help and the visual hint bar, including any pack links. Empty descriptions fall back to gear help. Concise Use / Settings / Includes overviews remain in More info.
- Browsing results: Shift+F10, Applications key, or the screen reader's Show menu action opens More info / Open on web. More info opens a scrollable overview without loading a tone; Tab reaches the readable text, full creator description toggle, web link and Close. Escape returns to the browsing result. Inspection remains available when a tone cannot be loaded.
- Loaded tones: Tab to the tone tile and press Enter to open its card. Shift+F10 or the Applications key opens the tile menu; Up/Down selects an action, Enter activates, Escape closes. Space on the tile is reserved for reordering.
- In the tone card, Tab to Info and press Enter. Focus moves to the readable overview (or the sign-in/retry prompt), including after details finish loading. Tab reaches More info and View on TONE3000. More info reveals a focusable paragraph containing the original creator text; Less info hides it. Verify NVDA reads both text paragraphs and announces the More info toggle state. Enter on View on TONE3000 opens the tone web page. Catalog details require sign-in; local file tones have no catalog Info button.
- In an opened tone, Favorites announces Add to favorites or Remove from favorites and its checked state. Enter toggles it while retaining focus. Signed-out favorites are disabled.
- Enter on EQ moves focus into its controls. Bypassed EQ focuses EQ Power first. In the slider view, Tab visits adjustable gain faders; Up/Down changes gain by 0.5 dB, Shift+Up/Down by 0.1 dB, and Home/End select the minimum/maximum gain. NVDA exposes each fader's band, frequency and gain value. Cut bands have no adjustable gain in this view.
- Opening Tuner focuses its reading. Verify note, octave, hertz, and cents flat/sharp or in tune (within 5 cents). The first detected note is spoken once per opening while the reading has focus; Enter or R repeats. Tab reaches Close tuner; Escape closes. Check no signal after the normal signal hold expires.

- Branch: in Stereo mode, activate Branch after an effect. Verify the other lane taps that effect, the control becomes Remove branch after, checked state changes, and focus stays there. Activate again to make lanes independent.
