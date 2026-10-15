# 0013. Apple's HIG Foundations are the standard for the icon

Status   - Accepted
Date     - 2026-10-15
Commits  - the `ccr-f2d9adc9-lanc9o` branch (version 1.1.0)

## Context

The user asked for every repository to follow Apple's Human Interface
Guidelines, as Transient Shuffler does, and sent the 19 Foundations pages.
Writing was done in 0012. Checked against the rest
([`docs/apple-hig-review.md`](../apple-hig-review.md)), the icon fell short in
three places:

- **Contrast.** A note name on an unlit circle, `#9EA3B2` on `#4C4F59`, was
  3.2:1. Apple's floor is 4.5:1 and its target for small text in a dark window
  7:1. Everything else already passed: the label 8.6:1, a chord name 13.6:1,
  the dark ink on every highlight 7.6:1 (Orange) to 18:1 (White).
- **Text size.** Names went down to 7 points and the label to 9 in a small
  dock. Apple's smallest text on the Mac is 10.
- **The keyboard.** Both menus opened only from a click.

## Decision

- **Unlit circles take two ramp colours**: `#3A404A` with `#DDE1E7` letters,
  8.0:1. Still a dark fill with light letters, so the notes outside the key
  recede; the open question of inverting the pair (CLAUDE.md, *Where ScaleView
  sits*) is untouched.
- **No text under 10 points** (`MIN_TEXT`). The icon already grows with its
  window, which is how the text is made bigger; a circle too small to hold a
  10-point "Bbb" (`MIN_NAME_RADIUS`, 9) shows no name rather than an unreadable
  one.
- **S or Return opens the scale list, O the options**, in the middle of the
  icon rather than under a pointer that may be elsewhere. The menus are the
  system's own, so the arrow keys and Return work inside them.
- Pro first, then Simple word for word, then the plugin.

## Consequences

- In a dock under about 160 pixels wide the circles carry no names; they did
  before at 7 points.
- Not met, because a `gfx` script can't: VoiceOver, Dynamic Type, Increase
  Contrast. Text grows with the window instead.
- Unverified in REAPER: whether REAPER hands S, O and Return to the window
  while it is focused (D and Esc already worked this way).

## Enforcement

Both suites: every note name is 7:1 or more on its own circle, and the label on
the background, with each of the six highlights; no `setfont` under 10 at four
window sizes; a 150x75 icon draws no names; S, Return and O open their menus
inside the window. Each of the seven rules was broken on purpose in each script
and the suite failed.
