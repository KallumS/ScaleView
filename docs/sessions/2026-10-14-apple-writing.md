# Session log - 14 October 2026: the menus and Apple's writing guidelines

| Repository | Branch |
| --- | --- |
| `KallumS/ScaleView-for-Reaper` | `claude/adoring-feynman-bihxp2` |
| `KallumS/ScaleView` (plugin) | `claude/adoring-feynman-bihxp2` |

## What was asked

A sweep of every repository so the text in each window follows Apple's
writing guidelines, as Noterator and Miderator now do (their decision 0053).
Here that is decision 0012.

## What changed

Very little needed to. Pro's two menus were already in title style, and the
label's two messages ("No scale selected", "MIDI input unavailable") are
sentence-case fragments, as they should be.

- **Random Scale** is now **Choose a Random Scale**: it acts, so it starts
  with a verb. Pro first, then Simple, word for word.
- The plugin had drifted from Pro: "Clear scale" and "Show note names" were
  in sentence case. They are **Clear Scale** and **Show Note Names** now, and
  its Random Scale is **Choose a Random Scale** as in Pro.
- Both scripts' `Version:` header said 1.0 while the index published 1.0.3;
  they say 1.0.4 now, the version this change is published as.

## Left alone

The scale, root and highlight-colour names (tables shared with the plugin
and the sister repos, and stored by name), the chord symbols, the ExtState
keys and the script file names. There is no item that asks for more input,
so no ellipsis anywhere.

## Verification

Both suites pass. The tests open the options menu by looking for "Choose a
Random Scale"; against the old scripts they fail twice each, so they see the
new words. The plugin's suites were rebuilt and run after its change.
