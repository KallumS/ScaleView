# ScaleView (plugin) against Apple's HIG Foundations

The first pass (October 2026) ported ScaleView for REAPER's decision 0013,
whose `docs/apple-hig-review.md` holds the page-by-page review: 7:1 note
names, the 10-point floor, S / Return / O from the keyboard, and the editor
read by VoiceOver as an image titled with the key and the chord. This file
records what the plugin did beyond that.

## Final sweep (16 October 2026, done 21 October)

Every item of the checklist of the 19 Foundations pages that applies to a
plugin editor: one resizable icon of twelve circles and a label, two menus,
no other controls. Verdicts were measured on the headless render, not
guessed. Version 1.1.0.

| Item | Page | Verdict |
| --- | --- | --- |
| Text 7:1 | Accessibility, Color | Met (first pass): names 8.0:1 and up, label 8.6:1, chord 13.6:1. |
| Shapes that mean something 3:1 | Accessibility, Color | **Fixed now**: an unlit circle was 1.4:1 on the background; a `#6D7581` rim makes it 3.2:1 with the fill kept dark. Lit circles 6.3:1 (Orange) to 15.0:1 (White). As ScaleView Pro. |
| Dark glyphs on a dark ground need an edge | Dark Mode | **Fixed now**, the same rim. |
| The played-note ring | Color | Met: `#FFF200`, unconditional, 12.8:1 on the background; the rim is a fill under the circle, never a second stroke. |
| The system font | Typography | **Fixed now on Windows**: Segoe UI when installed, else JUCE's default. **Not possible on the Mac** through JUCE's `Font`: San Francisco cannot be found by family name, and asking CoreText for `.AppleSystemUIFont` or `.SFNS…` by name gets Times Roman on recent macOS (its own console note says to use the system-font APIs, which `Font` does not expose). The Mac keeps JUCE's default sans-serif (Helvetica); Linux likewise. |
| Text floor 10 pt | Typography | Met (first pass). |
| Typefaces ≤ 2, no light weights | Typography | Met: one face, regular. |
| Menus lighter than the window | Dark Mode, Materials | **Fixed now**: the menus are JUCE's own drawing, not the system's, and were JUCE's teal-grey `#323E44` with a stripe every third line. Now `#2A2F37` with a `#3A404A` edge, plain; words `#DDE1E7` 10.2:1, the item under the pointer `#F2F4F7` on `#3A404A` 9.4:1; set in the same face as the icon. |
| Colour never alone | Color, Accessibility | Met: in the key is light against dark (and now rimmed against plain), played is a ring (a shape), the label changes words. |
| One meaning per colour | Color | Met: yellow is only the played-note ring. |
| Hit targets | Layout, Accessibility | Met: the whole editor is the target; a click anywhere opens a menu. Smallest size 200×100. |
| Minimum window size | Layout | Met: the constrainer stops at 200×100, where circles (radius 11) still carry names. |
| Mouse-only actions | Accessibility | Met (first pass): S / Return and O open the menus; JUCE's menus take the arrows, Return and Esc. |
| System shortcuts | Accessibility | Met: only S, O, Return; Command keys are left to the host. |
| Esc | Accessibility | Met: an open menu takes Esc; otherwise the host keeps it. |
| Focus ring, Tab, Full Keyboard Access | Accessibility | Not applicable: one focusable thing, the editor, with nothing inside to move between. |
| VoiceOver / Voice Control names | Accessibility | Met (first pass): an image titled "ScaleView: <key>, playing <chord>". Voice Control targets controls; the icon has none. |
| Accessibility Inspector audit | Accessibility | Only on a Mac: not run here. |
| Time limits | Accessibility | Met: none. |
| Motion, flashes, Reduce Motion | Motion | Not applicable: nothing moves. |
| Sound | Accessibility | Not applicable: audio passes through untouched. |
| Truncation | Typography | Not applicable: nothing is cut with "…"; "Bbb" is sized to fit its circle. |
| Destructive actions | Design principles | Not applicable: every choice is undone by choosing again. |
| Words ("we", "OK", jargon) | Writing | Met (Pro's menu words, decision 0012). |
| Window title | Layout | Not applicable: the host titles a plugin window. |
| Follow light / dark appearance | Dark Mode | User's decision: the house scheme is dark and its three colours are settled. |
| The system accent colour | Color | User's decision: the accent is the house yellow; highlights are the user's six. |
| White highlight, not off-white | Color, Dark Mode | User's decision: a small fill the user asked for, not a page. |
| App icon (Standalone) | App icons | Not applicable: the Standalone is for checking the plugin, not shipped; no artwork. |
| Microphone purpose string (Standalone) | Privacy | Not applicable to the plugin; noted: the Standalone passes audio through and declares no microphone purpose string. |
| Code signing | Privacy | Not possible here: the AU and signing need a Mac. |
| SF Symbols, Right to left, visionOS | — | Not applicable: no symbols, no reading direction, no spatial UI. |

Checks, in `Tests/TestProcessorMidi.cpp`: every circle's outer edge 3:1 with
all six highlights (unlit and lit), the face picked only when installed and
used by the editor, and the menus' colours, contrast and plain edged
background. Seven sabotages in a scratch copy (no rim; a rim too dark; a face
asked for without checking it is installed; a face forced; the menu look not
set; JUCE's striped background; dim menu words), all caught. One that changes
nothing on Linux was also tried - reading the installed faces when none is
wanted - and is harmless.

Only a DAW and a Mac or Windows machine can confirm: Segoe UI in the icon and
the menus on Windows, how "Bbb" fits its circle in it at 10 points, the
menus' look in each host, and an Accessibility Inspector pass.
