# PinkReader Desktop — Reddit Client for Desktop

A native Reddit client built with wxWidgets 3.2 and C++20. Browse subreddits, view images, and search posts with a multi-pane GUI.

## Quick Start

```bash
mkdir build && cd build
cmake .. && make
./pinkreader-desktop
```

## Features

- Browse r/popular, r/all, and any subreddit
- Post list with thumbnails, scores, comment counts
- Image viewer panel for post images
- Search panel with history
- Sort by hot, new, top, rising, controversial
- Status bar with double-click-to-copy
- Keyboard shortcuts (F5 refresh)

## Panels

- **Post List** — scrollable list of posts from the current subreddit
- **Image Viewer** — displays the selected post's image
- **Search** — type a subreddit name or search query

## Architecture

- C++20 + wxWidgets 3.2 for the GUI
- libcurl for Reddit JSON API access
- wxAUI for dockable panel layout

## Build

```bash
mkdir build && cd build
cmake .. && make
```
Requires: wxWidgets 3.2+, CMake 3.16+, GCC 10+ or Clang 12+, libcurl
