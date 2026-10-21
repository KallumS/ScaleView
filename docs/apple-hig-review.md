# Review against Apple's Human Interface Guidelines: Foundations

ScaleView Pro and Simple checked against all 19 Foundations pages (the user's
copy, October 2026), the same review Transient Shuffler had. Writing was done
first ([0012](decisions/0012-menu-text-follows-apples-writing-guidelines.md));
this covers the rest ([0013](decisions/0013-apples-hig-foundations.md)).

The icon is drawn with REAPER's `gfx` API, so some guidance cannot apply: a
script cannot reach VoiceOver, Dynamic Type or Increase Contrast. Where a
guideline is not met for that reason, this says so.

## What changed (1.1.0)

| Guideline | Before | Now |
| --- | --- | --- |
| **Accessibility, Dark Mode: contrast** | A note name on an unlit circle 3.2:1. | `#DDE1E7` on `#3A404A`, 8.0:1. Every other letter already passed. |
| **Typography: minimum size** | Names down to 7 points, the label to 9. | Nothing under 10, Apple's smallest on the Mac; a circle too small for that shows no name. |
| **Accessibility: the keyboard alone** | Menus by click only. | **S** or **Return** opens the scale list, **O** the options. |

## Already met

- **Larger text.** The whole icon, text included, grows with its window.
- **More than colour alone.** A note in the key is a bright circle with dark
  letters, one outside it a dark circle; a played note is ringed. Light and
  shape carry it, not hue.
- **Motion.** None. **No time-boxed elements.** None.
- **Dark Mode.** Always dark: the house scheme, and Apple allows it for apps
  built around media.
- **Writing.** Decision 0012.
- **Privacy.** Reads only REAPER's MIDI input history (Pro); saves only its
  settings in REAPER and the scale in the project.

## Not met, because a `gfx` script can't

- **VoiceOver, Voice Control, Switch Control.**
- **Dynamic Type and Increase Contrast.**
- **The system font.** `gfx` takes a face by name; Arial was used. Since
  1.2.0 the system's own face is asked for and kept when REAPER finds it
  (*Final sweep* below).

## Not relevant

Layout of controls (there are none but the menus), materials, app icons, SF
Symbols, images, spatial layout, immersive experiences.

## Unverified in REAPER

- Whether S, O and Return reach the window as D and Esc do.

## Checked by

Both suites (`tests/test_scaleview_pro.lua`, `tests/test_scaleview_simple.lua`).

## Final sweep (16 October 2026, done 21 October)

Every item tagged implementable in the checklist of the 19 pages, for an icon
of twelve circles, a label and two native menus.

| Item | Verdict |
| --- | --- |
| Text 7:1 | Met (0013): names 8:1 and up, label 8.6:1, chord 13.6:1. |
| Non-text contrast 3:1 | **Fixed now**: unlit circles 1.4:1 → a `#6D7581` rim, 3.2:1. Lit circles 6.3:1 (Orange) and up; the ring 12.8:1. |
| Dark glyphs on a dark ground need an edge | **Fixed now**, the same rim. |
| The system font | **Fixed now**: San Francisco, then Segoe UI, if `gfx.getfont` says so; else Arial. |
| Text floor 10 pt | Met (0013). |
| Typefaces ≤ 2, no light weights | Met: one face, regular. |
| Colour never alone | Met: in the key is light against dark, played is a ring (shape); a chord label differs in words. |
| One meaning per colour | Met: the yellow is only the played-note ring. |
| Hit targets | Met: every click opens a menu - a circle the scale list, anywhere else the options; nothing is too small to hit. |
| Mouse-only actions | Met (0013): S/Return and O; D docks; menus are the system's own. |
| System shortcuts | Met: only S, O, D, Return, Esc; no Command keys taken. |
| Esc | Met: closes the icon; inside a menu the system's own menu takes it. |
| Time limits | Met: none. The 0.25 s menu settle only ignores a stray click. |
| Motion, flashes, Reduce Motion | Not applicable: nothing moves. |
| Sound | Not applicable: plays nothing. |
| Window title | Met: "ScaleView Pro" / "ScaleView Simple"; the key is on the icon itself. REAPER's documentation gives no way to retitle a window, so the key is not added. |
| Minimum window size | Not applicable: no controls to lose; a tiny icon drops only the names. |
| Truncation | Not applicable: nothing is cut with "…". |
| Popups lighter than the window | Not applicable: the menus are the system's. |
| Destructive actions | Not applicable: every choice is undone by choosing again. |
| Words ("we", "OK", jargon) | Met (0012): menu items checked. |
| White, not off-white | User's decision: White is a highlight the user asked for, a small fill, not a page. |
| Focus ring, Tab, hover states | Not applicable: no controls inside the window. |
| VoiceOver, Dynamic Type, Increase Contrast | Not possible from `gfx` (above). |

For the plugin (`KallumS/ScaleView`, not changed here): the same rim on an
unlit circle would bring it to 3:1 too; it already uses the system face.
