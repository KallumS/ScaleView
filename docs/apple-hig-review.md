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
- **The system font.** `gfx` takes a face by name; Arial is used.

## Not relevant

Layout of controls (there are none but the menus), materials, app icons, SF
Symbols, images, spatial layout, immersive experiences.

## Unverified in REAPER

- Whether S, O and Return reach the window as D and Esc do.

## Checked by

Both suites (`tests/test_scaleview_pro.lua`, `tests/test_scaleview_simple.lua`).
