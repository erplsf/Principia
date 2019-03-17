﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace principia {
namespace ksp_plugin_adapter {

internal class Dialog : IConfigNode {
  public String Message {
    set {
      message_ = value;
      UnityEngine.Debug.LogError(message_);
    }
  }

  public void Show() {
    UnityEngine.GUI.skin = null;
    rectangle_ = UnityEngine.GUILayout.Window(
        id         : this.GetHashCode(),
        screenRect : rectangle_,
        func       : (int id) => {
          using (new VerticalLayout())
          {
            UnityEngine.GUILayout.TextArea(message_ ?? "SHOW WITHOUT MESSAGE");
          }
          UnityEngine.GUI.DragWindow();
        },
        text       : "Principia");
    WindowUtilities.EnsureOnScreen(ref rectangle_);
  }

  void IConfigNode.Load(ConfigNode node) {
    String x_value = node.GetAtMostOneValue("x");
    if (x_value != null) {
      rectangle_.x = System.Convert.ToSingle(x_value);
    }
    String y_value = node.GetAtMostOneValue("y");
    if (y_value != null) {
      rectangle_.y = System.Convert.ToSingle(y_value);
    }
    message_ = node.GetAtMostOneValue("message");
  }

  void IConfigNode.Save(ConfigNode node) {
    node.SetValue("x", rectangle_.x, createIfNotFound : true);
    node.SetValue("y", rectangle_.y, createIfNotFound : true);
    if (message_ != null) {
      node.SetValue("message", message_, createIfNotFound : true);
    }
  }

  private static readonly float min_width_ = 500;

  // The message shown, if any.
  private String message_;
  private UnityEngine.Rect rectangle_ =
      new UnityEngine.Rect(x      : (UnityEngine.Screen.width - min_width_) / 2,
                           y      : UnityEngine.Screen.height / 3,
                           width  : min_width_,
                           height : 0);
}

}  // namespace ksp_plugin_adapter
}  // namespace principia