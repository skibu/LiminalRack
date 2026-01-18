# UI Readme

The UI code for Rack is pretty difficult to understand and has incredibly few comments. 
Therefore this document explains a few of the concepts.

### Overview
[Wikipedia article on Scene Graph](https://en.wikipedia.org/wiki/Scene_graphhttps://en.wikipedia.org/wiki/Scene_graph)
explains the tree based way that the graphics software works.

### Widget
The fundamental UI object is actually widget/Widget.cpp . 
From the documentation for the Widget class, a Widget is a node in the 2D [scene graph](https://en.wikipedia.org/wiki/Scene_graph).
The bounding box of a Widget is a rectangle specified by `box` relative to their parent.
The appearance is defined by overriding `draw()`, and the behavior is defined by overriding `step()` and `on*()` event handlers.

MenuLabel - A non-selectable label that can be an item in a menu

### Class hierarchy for Menu widgets
```
Widget - box, parent, children, visible
  OpaqueWidget -
    MenuBar - infoLabel
    Menu - childMenu, activeEntry
    MenuOverlay - bgcolor
    MenuEntry - sets box.size = math::Vec(0, rack::settings::bndWidgetHeight)
      MenuItem - text, rightText, disabled
      MenuLabel (non-clickable) - text
```

But there are also the buttons that go in the menu bar, which have a completely different heirarchy.
```
ui::Button
  MenuButton (in MenuBar.cpp)
    FileButton
    EditButton
    ViewButton
    EngineButton
    LibraryButton
    HelpButton
```
