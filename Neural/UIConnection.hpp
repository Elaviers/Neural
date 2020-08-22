#pragma once
#include <ELUI/Element.hpp>
#include "UINode.hpp"

class UIConnection : public UIElement
{
private:
	SharedPointer<const Texture> _texture;
	SharedPointer<const Font> _font;

	SharedPointer<const UINode> _from;
	SharedPointer<const UINode> _to;

	bool _centreHover;

	float _size;
	float _lineWidth;

	float _weight;

	Colour _lineColour;
	Colour _lineColourHover;
	UIColour _centreColour;
	UIColour _centreColourHover;
	UIColour _fontColour;

	FunctionPointer<void, UIConnection&> _onRightClick;

	bool _PointOverlapsLine(const Vector2& point) const;
	bool _PointOverlapsCentre(const Vector2& point) const;

public:
	UIConnection(UIElement* parent) : UIElement(parent), _centreHover(0.f), _lineWidth(3.f), _size(64.f), _weight(1.f), 
		_lineColour(Colour::Grey), _lineColourHover(Colour::White), _centreColour(Colour::Grey, Colour::Black), _centreColourHover(Colour::White, Colour::Grey),
		_fontColour(Colour::White) { SetZ(1.f); }
	~UIConnection() {}

	const SharedPointer<const UINode>& GetFrom() const { return _from; }
	const SharedPointer<const UINode>& GetTo() const { return _to; }
	const SharedPointer<const Texture>& GetTexture() const { return _texture; }
	const SharedPointer<const Font>& GetFont() const { return _font; }
	float GetSize() const { return _size; }
	float GetLineWidth() const { return _lineWidth; }
	float GetWeight() const { return _weight; }
	const Colour& GetLineColour() const { return _lineColour; }
	const Colour& GetLineColourHover() const { return _lineColourHover; }
	const UIColour& GetCentreColour() const { return _centreColour; }
	const UIColour& GetCentreColourHover() const { return _centreColourHover; }
	const UIColour& GetFontColour() const { return _fontColour; }

	void SetFrom(const SharedPointer<const UINode>& from) { _from = from; }
	void SetTo(const SharedPointer<const UINode>& to) { _to = to; }
	void SetTexture(const SharedPointer<const Texture>& texture) { _texture = texture; }
	void SetFont(const SharedPointer<const Font>& font) { _font = font; }
	void SetSize(float size) { _size = size; }
	void SetLineWidth(float lineWidth) { _lineWidth = lineWidth; }
	void SetWeight(float weight) { _weight = weight; }
	void SetLineColour(const Colour& lineColour) { _lineColour = lineColour; }
	void SetLineColourHover(const Colour& lineColourHover) { _lineColourHover = lineColourHover; }
	void SetCentreColour(const UIColour& centreColour) { _centreColour = centreColour; }
	void SetCentreColourHover(const UIColour& centreColourHover) { _centreColourHover = centreColourHover; }
	void SetFontColour(const UIColour& fontColour) { _fontColour = fontColour; }
	void SetOnRightClick(const FunctionPointer<void, UIConnection&>& onRightClick) { _onRightClick = onRightClick; }

	virtual bool OverlapsPoint(float x, float y) const override;
	
	virtual void Render(RenderQueue&) const override;
	
	virtual bool OnKeyUp(EKeycode) override;
	virtual bool OnKeyDown(EKeycode) override;
	virtual bool OnMouseMove(float x, float y, bool blocked) override;

	virtual void OnHoverStart() override;
	virtual void OnHoverStop() override;
};

