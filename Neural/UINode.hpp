#pragma once
#include "NetPerceptron.hpp"
#include <ELGraphics/Font.hpp>
#include <ELUI/Panel.hpp>

class UINode : public UIPanel
{
private:
	bool _edgeHover;
	bool _dragging;
	float _dragOffsetX;
	float _dragOffsetY;

	UIColour _colourDefault;
	UIColour _colourHover;
	UIColour _colourEdgeHover;

	FunctionPointer<void, UINode&> _onEdgeMouseDown;
	FunctionPointer<void, UINode&> _onHoverMouseUp;
	FunctionPointer<void, UINode&> _onDeleteRequest;

	SharedPointer<const Font> _font;

public:
	SharedPointer<NetPerceptron> perceptron;

	UINode(UIElement* parent = nullptr);
	virtual ~UINode() {}

	const UIColour& GetColourDefault() const { return _colourDefault; }
	const UIColour& GetColourHover() const { return _colourHover; }
	const UIColour& GetColourEdgeHover() const { return _colourEdgeHover; }
	const SharedPointer<const Font>& GetFont() const { return _font; }

	void SetColourDefault(const UIColour& colour) { _colourDefault = colour; if (!_hover) SetColour(_colourDefault); }
	void SetColourHover(const UIColour& colour) { _colourHover = colour; if (_hover && !_edgeHover) SetColour(_colourHover); }
	void SetColourEdgeHover(const UIColour& colour) { _colourEdgeHover = colour; if (_hover && _edgeHover) SetColour(_colourEdgeHover); }
	void SetFont(const SharedPointer<const Font>& font) { _font = font; }

	void SetOnEdgeMouseDown(const FunctionPointer<void, UINode&>& onEdgeMouseDown) { _onEdgeMouseDown = onEdgeMouseDown; }
	void SetOnHoverMouseUp(const FunctionPointer<void, UINode&>& onHoverMouseUp) { _onHoverMouseUp = onHoverMouseUp; }
	void SetOnDeleteRequest(const FunctionPointer<void, UINode&>& onDeleteRequest) { _onDeleteRequest = onDeleteRequest; }

	virtual bool OverlapsPoint(float x, float y) const override;

	virtual void Render(RenderQueue&) const override;

	virtual bool OnKeyUp(bool blocked, EKeycode) override;
	virtual bool OnKeyDown(bool blocked, EKeycode) override;
	virtual bool OnMouseMove(bool blocked, float mouseX, float mouseY) override;

	virtual void OnHoverStart() override;
	virtual void OnHoverStop() override;
};

