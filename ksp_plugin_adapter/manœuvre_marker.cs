﻿using System;
using System.Collections;

namespace principia {
namespace ksp_plugin_adapter {

internal class ManœuvreMarker : UnityEngine.MonoBehaviour {
  public bool is_hovered { get; private set; } = false;
  public bool is_dragged { get; private set; } = false;
  public bool is_pinned { get; private set; } = false;
  // Note that |is_hovered| alone does not imply |is_interacting|: the cursor
  // may move off the marker while still dragging.
  public bool is_interacting => is_hovered || is_dragged;

  // As mouse events are sent before |Update|, we compute this state there.
  // We clear it as late as possible, in |WaitForEndOfFrame|.
  // In particular, the value is meaningful at |BetterLateThanNeverLateUpdate|,
  // during which it is consumed by node markers.
  public static bool has_interacting_marker { get; private set; }

  // Construct the geometry of the marker when instantiated.
  // 1. Build the mesh of the marker (base 'bulb' & one cylinder for each axis).
  // 2. Attach a spherical collider (centred on the bulb) to the gameobject for
  //    mouse detection.
  // 3. Add an empty text object for the caption, 'below' the marker. Note that
  //    this is positioned in UI space (which is distinct from screen
  //    coordinates) rather than in scaled space.
  // 4. Configure the layers of the objects appropriately.
  // Note that the 'base' scale of the marker is such that it is circumscribed
  // by a sphere one scaled-space unit in radius.
  public void Awake() {
    base_ = MakeTrihedronBase();
    tangent_ = MakeTrihedronArrow();
    normal_ = MakeTrihedronArrow();
    binormal_ = MakeTrihedronArrow();

    base_.transform.SetParent(gameObject.transform, false);
    tangent_.transform.SetParent(gameObject.transform, false);
    normal_.transform.SetParent(gameObject.transform, false);
    binormal_.transform.SetParent(gameObject.transform, false);

    var collider = gameObject.AddComponent<UnityEngine.SphereCollider>();
    collider.radius = collider_radius;

    transform.SetLayerRecursive((int)PrincipiaPluginAdapter.UnityLayers.
                                    Atmosphere);

    caption_ = MakeCaption();
    caption_.transform.SetParent(transform, false);
    caption_text_ = caption_.GetComponentInChildren<TMPro.TextMeshPro>();
    caption_.SetLayerRecursive((int)PrincipiaPluginAdapter.UnityLayers.UI);

    Disable();
  }

  // Call on each frame (at or later than |Update|) to set the state of the marker.
  public void Render(Vector3d world_position,
                     Vector3d initial_plotted_velocity,
                     int index,
                     NavigationManoeuvreFrenetTrihedron trihedron,
                     Func<NavigationManoeuvre> get_manœuvre,
                     Action<Burn> modify_burn) {
    initial_plotted_velocity_ = initial_plotted_velocity;
    index_ = index;
    get_manœuvre_ = get_manœuvre;
    modify_burn_ = modify_burn;

    transform.position = ScaledSpace.LocalToScaledSpace(world_position);

    tangent_.transform.localRotation = UnityEngine.Quaternion.FromToRotation(
        UnityEngine.Vector3.up,
        (Vector3d)trihedron.tangent);
    normal_.transform.localRotation = UnityEngine.Quaternion.FromToRotation(
        UnityEngine.Vector3.up,
        (Vector3d)trihedron.normal);
    binormal_.transform.localRotation = UnityEngine.Quaternion.FromToRotation(
        UnityEngine.Vector3.up,
        (Vector3d)trihedron.binormal);

    normalized_scale_ =
        (ScaledSpace.ScaledToLocalSpace(MapView.MapCamera.transform.position) -
         world_position).magnitude *
        rescale_factor *
        ScaledSpace.InverseScaleFactor;
    UpdateScale();

    UpdateColours();
    UpdateCaption();

    gameObject.SetActive(true);
  }

  // Disable this marker and reset its user-interaction state so that it is
  // ready for reuse.
  public void Disable() {
    gameObject.SetActive(false);
    is_hovered = false;
    is_dragged = false;
    is_pinned = false;
  }

  private void SetColour(UnityEngine.GameObject game_object,
                         UnityEngine.Color colour) {
    if (is_interacting) {
      UnityEngine.Color.RGBToHSV(colour, out float h, out float s, out float v);
      colour
        = UnityEngine.Color.HSVToRGB(h, s, v * hover_luminosity_boost, hdr: true);
    }
    game_object.GetComponentInChildren<UnityEngine.Renderer>().material.color
      = colour;
  }

  // Brighten the marker when interacting.
  private void UpdateColours() {
    SetColour(base_, Style.ManœuvreMarkerBase);
    SetColour(tangent_, Style.Tangent);
    SetColour(normal_, Style.Normal);
    SetColour(binormal_, Style.Binormal);
  }

  // Open/close/pin the caption based on hover/pin state.
  private void UpdateCaption() {
    if (!is_interacting && !is_pinned) {
      caption_.SetActive(false);
      return;
    }
    caption_.SetActive(true);
    caption_.transform.position = ScaledToUIPosition(transform.position).position;

    // Note that we check for |is_interacting| rather than |is_pinned|; this is
    // so that the brighter hover colour is applied in the 'hovered and pinned'
    // state.
    if (is_interacting) {
      caption_text_.color = Style.MarkerCaption;
    } else {
      caption_text_.color = Style.MarkerCaptionPinned;
    }

    var manœuvre = get_manœuvre_();
    var burn = manœuvre.burn;
    var caption = L10N.CacheFormat(
      "#Principia_MapNode_ManœuvreCaption",
      index_ + 1,
      ((Vector3d)burn.delta_v).magnitude.ToString("0.000"),
      manœuvre.duration.ToString("0.0"),
      "T" + new PrincipiaTimeSpan(
              Planetarium.GetUniversalTime() - burn.initial_time).
          Format(with_leading_zeroes: false, with_seconds: true).
          Replace(Culture.culture.NumberFormat.NegativeSign, "-")
    );
    caption_text_.SetText(caption);
  }

  // Increase the size of the marker (with an animation) when interacting.
  private void UpdateScale() {
    current_hover_scale_multiplier_ = UnityEngine.Mathf.MoveTowards(
        current_hover_scale_multiplier_,
        is_interacting ? hover_scale_multiplier : 1f,
        0.0625f);
    var resulting_scale = normalized_scale_ * current_hover_scale_multiplier_;
    transform.localScale = Vector3d.one * resulting_scale;
    // Inversely scale the caption, since it is drawn in UI coordinates and thus
    // does not require additional dynamic scaling.
    caption_.transform.localScale = Vector3d.one / resulting_scale;
  }

  private UnityEngine.Vector3 ScreenManœuvrePosition() =>
      ScaledToFlattenedScreenPosition(transform.position);

#region Mouse Events
  public void OnMouseEnter() {
    is_hovered = true;
  }

  public void OnMouseDown() {
    mouse_offset_at_click_ =
        UnityEngine.Input.mousePosition - ScreenManœuvrePosition();
    is_dragged = true;
  }

  public void OnMouseOver() {
    if (Mouse.Right.GetButtonUp()) {
      is_pinned = !is_pinned;
    }
  }

  public void OnMouseDrag() {
    var mouse_offset_now =
        UnityEngine.Input.mousePosition - ScreenManœuvrePosition();
    var mouse_displacement = mouse_offset_now - mouse_offset_at_click_;

    // TODO(egg): This is cheesy. Could we do it in the C++?
    var screen_velocity =
        (ScaledToFlattenedScreenPosition(
             transform.position +
             ScaledSpace.InverseScaleFactor * initial_plotted_velocity_) -
         ScreenManœuvrePosition());

    var Δt = UnityEngine.Vector3.Dot(screen_velocity, mouse_displacement) /
              screen_velocity.sqrMagnitude;
    var burn = get_manœuvre_().burn;
    burn.initial_time += Δt;
    modify_burn_(burn);
  }

  public void OnMouseUp() {
    is_dragged = false;
  }

  public void OnMouseExit() {
    is_hovered = false;
  }
#endregion Mouse Events

  // Accumulate and reset the interactivity states of all active markers.
  public void Update() {
    has_interacting_marker |= is_interacting;

    // Only run one copy of this coroutine.
    if (index_ == 0) {
      StartCoroutine(ResetInteractivityStatus());
    }
  }

  private static IEnumerator ResetInteractivityStatus() {
    yield return new UnityEngine.WaitForEndOfFrame();
    has_interacting_marker = false;
  }

  private static UnityEngine.GameObject MakeTrihedronBase() {
    var sphere = UnityEngine.GameObject.CreatePrimitive(
        UnityEngine.PrimitiveType.Sphere);
    Destroy(sphere.GetComponent<UnityEngine.Collider>());
    sphere.transform.localScale *= base_diameter;
    sphere.GetComponent<UnityEngine.Renderer>().material =
        new UnityEngine.Material(marker_material);
    return sphere;
  }

  private static UnityEngine.GameObject MakeTrihedronArrow() {
    var arrow = new UnityEngine.GameObject("arrow");
    var cylinder = UnityEngine.GameObject.CreatePrimitive(
        UnityEngine.PrimitiveType.Cylinder);
    Destroy(cylinder.GetComponent<UnityEngine.Collider>());
    cylinder.transform.SetParent(arrow.transform, false);
    cylinder.transform.localScale
      = new UnityEngine.Vector3(arrow_diameter, 0.5f, arrow_diameter);
    cylinder.transform.localPosition = new UnityEngine.Vector3(0f, 0.5f, 0f);
    cylinder.GetComponent<UnityEngine.Renderer>().material =
        new UnityEngine.Material(marker_material);
    return arrow;
  }

  private static UnityEngine.GameObject MakeCaption() {
    var caption = new UnityEngine.GameObject("caption");
    var caption_inner = new UnityEngine.GameObject("TMPro");
    var text = caption_inner.AddComponent<TMPro.TextMeshPro>();
    var inner_transform = caption_inner.GetComponent<UnityEngine.RectTransform>();
    inner_transform.SetParent(caption.transform, false);
    inner_transform.localPosition = new UnityEngine.Vector3(0f, -10f, 0f);
    inner_transform.sizeDelta = new UnityEngine.Vector2(300f, 1f);
    text.font = UISkinManager.TMPFont;
    text.fontSize = 140;
    text.alignment = TMPro.TextAlignmentOptions.Top;
    return caption;
  }

  private static UnityEngine.Vector3 ScaledToFlattenedScreenPosition(
      Vector3d scaled_position) {
    var position = PlanetariumCamera.Camera.WorldToScreenPoint(scaled_position);
    position.z = 0;
    return position;
  }

  // From |MapNode.OnUpdatePositionToUI|.
  private static (UnityEngine.Vector3 position, bool visible) ScaledToUIPosition(
    Vector3d scaled_position) {
      bool visible = false;
      var position = KSP.UI.Screens.Mapview.MapViewCanvasUtil.ScaledToUISpacePos(
          scaled_position,
          ref visible,
          KSP.UI.Screens.Mapview.MapNode.zSpaceEasing,
          KSP.UI.Screens.Mapview.MapNode.zSpaceMidpoint,
          KSP.UI.Screens.Mapview.MapNode.zSpaceUIStart,
          KSP.UI.Screens.Mapview.MapNode.zSpaceLength);
    return (position, visible);
  }

  private UnityEngine.GameObject base_;
  private UnityEngine.GameObject tangent_;
  private UnityEngine.GameObject normal_;
  private UnityEngine.GameObject binormal_;

  private UnityEngine.GameObject caption_;
  private TMPro.TextMeshPro caption_text_;

  private double normalized_scale_;
  private float current_hover_scale_multiplier_;

  private UnityEngine.Vector3 mouse_offset_at_click_;

  private Vector3d initial_plotted_velocity_;
  private int index_;
  private Func<NavigationManoeuvre> get_manœuvre_;
  private Action<Burn> modify_burn_;

  // In scaled space units:
  private const float collider_radius = 0.75f;
  private const float base_diameter = 0.3f;
  private const float arrow_diameter = 0.15f;

  private const double rescale_factor = 0.015d;

  private const float hover_luminosity_boost = 1.375f;
  private const float hover_scale_multiplier = 1.25f;

  private static UnityEngine.Material marker_material_;
  public static UnityEngine.Material marker_material {
    get {
      if (marker_material_ == null) {
        marker_material_ =
            new UnityEngine.Material(UnityEngine.Shader.Find("Unlit/Color"));
      }
      return marker_material_;
    }
  }
}

}  // namespace ksp_plugin_adapter
}  // namespace principia
