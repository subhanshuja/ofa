// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/aura/pointer_watcher_adapter.h"

#include "ash/shell.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/widget.h"

namespace ash {

PointerWatcherAdapter::PointerWatcherAdapter() {
  Shell::GetInstance()->AddPreTargetHandler(this);
}

PointerWatcherAdapter::~PointerWatcherAdapter() {
  Shell::GetInstance()->RemovePreTargetHandler(this);
}

void PointerWatcherAdapter::AddPointerWatcher(
    views::PointerWatcher* watcher,
    views::PointerWatcherEventTypes events) {
  // We only allow a watcher to be added once. That is, we don't consider
  // the pair of |watcher| and |events| unique, just |watcher|.
  DCHECK(!non_move_watchers_.HasObserver(watcher));
  DCHECK(!move_watchers_.HasObserver(watcher));
  DCHECK(!drag_watchers_.HasObserver(watcher));
  if (events == views::PointerWatcherEventTypes::DRAGS)
    drag_watchers_.AddObserver(watcher);
  else if (events == views::PointerWatcherEventTypes::MOVES)
    move_watchers_.AddObserver(watcher);
  else
    non_move_watchers_.AddObserver(watcher);
}

void PointerWatcherAdapter::RemovePointerWatcher(
    views::PointerWatcher* watcher) {
  non_move_watchers_.RemoveObserver(watcher);
  move_watchers_.RemoveObserver(watcher);
  drag_watchers_.RemoveObserver(watcher);
}

void PointerWatcherAdapter::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() != ui::ET_MOUSE_PRESSED &&
      event->type() != ui::ET_MOUSE_RELEASED &&
      event->type() != ui::ET_MOUSE_MOVED &&
      event->type() != ui::ET_MOUSEWHEEL &&
      event->type() != ui::ET_MOUSE_CAPTURE_CHANGED &&
      event->type() != ui::ET_MOUSE_DRAGGED)
    return;

  DCHECK(ui::PointerEvent::CanConvertFrom(*event));
  NotifyWatchers(ui::PointerEvent(*event), *event);
}

void PointerWatcherAdapter::OnTouchEvent(ui::TouchEvent* event) {
  if (event->type() != ui::ET_TOUCH_PRESSED &&
      event->type() != ui::ET_TOUCH_RELEASED &&
      event->type() != ui::ET_TOUCH_MOVED)
    return;

  DCHECK(ui::PointerEvent::CanConvertFrom(*event));
  NotifyWatchers(ui::PointerEvent(*event), *event);
}

gfx::Point PointerWatcherAdapter::GetLocationInScreen(
    const ui::LocatedEvent& event) const {
  gfx::Point location_in_screen;
  if (event.type() == ui::ET_MOUSE_CAPTURE_CHANGED) {
    location_in_screen = display::Screen::GetScreen()->GetCursorScreenPoint();
  } else {
    aura::Window* target = static_cast<aura::Window*>(event.target());
    location_in_screen = event.location();
    aura::client::GetScreenPositionClient(target->GetRootWindow())
        ->ConvertPointToScreen(target, &location_in_screen);
  }
  return location_in_screen;
}

views::Widget* PointerWatcherAdapter::GetTargetWidget(
    const ui::LocatedEvent& event) const {
  aura::Window* window = static_cast<aura::Window*>(event.target());
  return views::Widget::GetTopLevelWidgetForNativeView(window);
}

void PointerWatcherAdapter::NotifyWatchers(
    const ui::PointerEvent& event,
    const ui::LocatedEvent& original_event) {
  const gfx::Point screen_location(GetLocationInScreen(original_event));
  views::Widget* target_widget = GetTargetWidget(original_event);
  FOR_EACH_OBSERVER(
      views::PointerWatcher, drag_watchers_,
      OnPointerEventObserved(event, screen_location, target_widget));
  if (original_event.type() != ui::ET_TOUCH_MOVED &&
      original_event.type() != ui::ET_MOUSE_DRAGGED) {
    FOR_EACH_OBSERVER(
        views::PointerWatcher, move_watchers_,
        OnPointerEventObserved(event, screen_location, target_widget));
  }
  if (event.type() != ui::ET_POINTER_MOVED) {
    FOR_EACH_OBSERVER(
        views::PointerWatcher, non_move_watchers_,
        OnPointerEventObserved(event, screen_location, target_widget));
  }
}

}  // namespace ash