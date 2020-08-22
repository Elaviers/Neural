#include "UINode.hpp"
#include <ELGraphics/RenderQueue.hpp>

UINode::UINode(UIElement* parent) : 
	UIPanel(parent), 
	_colourDefault(UIColour(Colour::Grey, Colour::Blue)), 
	_colourEdgeHover(UIColour(Colour::White, Colour::Blue)),
	_colourHover(UIColour(Colour::White, Colour((byte)50, 50, 255)))
{
	SetBorderSize(0.f);
	SetColour(_colourDefault);
}

bool UINode::OverlapsPoint(float x, float y) const
{
	float r = _absoluteBounds.w / 2.f;
	float dx = x - (_absoluteBounds.x + r);
	float dy = y - (_absoluteBounds.y + _absoluteBounds.h / 2.f);
	float dist2 = (dx * dx) + (dy * dy);
	return dist2 < (r * r);
}

void UINode::Render(RenderQueue& queue) const
{
	UIPanel::Render(queue);

	if (_font)
	{
		String string = String::FromFloat(perceptron->GetBias()).GetData();

		RenderEntry& e = queue.NewDynamicEntry(ERenderChannels::UNLIT);
		e.AddSetColour(Colour::White);
		_font->RenderString(
			e,
			string.GetData(), 
			Transform(
				Vector3(_absoluteBounds.x + _absoluteBounds.w / 2.f - _font->CalculateStringWidth(string.GetData(), 32.f) / 2.f, _absoluteBounds.y + _absoluteBounds.h / 2.f - 16.f), 
				Rotation(), 
				Vector3(32.f, 32.f, 1.f)
			)
		);
	}
}

bool UINode::OnKeyUp(EKeycode key)
{
	if (_hover && key == EKeycode::MOUSE_RIGHT)
	{
		_onDeleteRequest(*this);
		return true;
	}

	return false;
}

bool UINode::OnKeyDown(EKeycode key)
{
	if (_hover)
	{
		switch (key)
		{
		case EKeycode::MOUSE_SCROLLDOWN:
			perceptron->SetBias(perceptron->GetBias() - 1);
			break;

		case EKeycode::MOUSE_SCROLLUP:
			perceptron->SetBias(perceptron->GetBias() + 1);
			break;
		default:
			return false;
		}

		return true;
	}

	return false;
}

bool UINode::OnMouseUp()
{
	_dragging = false;

	if (_hover)
	{
		_onHoverMouseUp(*this);
		return true;
	}

	return false;
}

bool UINode::OnMouseDown()
{
	if (_hover)
	{
		if (_edgeHover)
		{
			_onEdgeMouseDown.TryCall(*this);
		}
		else
		{
			_dragging = true;
		}

		return true;
	}

	return false;
}

bool UINode::OnMouseMove(float mouseX, float mouseY, bool blocked)
{
	if (blocked)
	{
		if (_hover)
		{
			_hover = false;
			OnHoverStop();
		}

		return false;
	}

	float r = _absoluteBounds.w / 2.f;
	float dx = mouseX - (_absoluteBounds.x + r);
	float dy = mouseY - (_absoluteBounds.y + _absoluteBounds.h / 2.f);
	float distFromEdge2 = (dx * dx) + (dy * dy) - (r * r);
	bool hover = distFromEdge2 <= 0.f;
	bool edgeHover = distFromEdge2 > -1024.f;
	
	if ((hover != _hover || edgeHover != _edgeHover))
	{
		_hover = hover;
		_edgeHover = edgeHover;
		_hover ? OnHoverStart() : OnHoverStop();
	}

	if (_dragging)
	{
		SetX(UICoord(0.f, mouseX + _dragOffsetX));
		SetY(UICoord(0.f, mouseY + _dragOffsetY));
	}
	else if (_hover)
	{
		_dragOffsetX = _absoluteBounds.x - mouseX;
		_dragOffsetY = _absoluteBounds.y - mouseY;
	}

	return _hover;
}

void UINode::OnHoverStart()
{
	if (_edgeHover)
		SetColour(_colourEdgeHover);
	else
		SetColour(_colourHover);
}

void UINode::OnHoverStop()
{
	SetColour(_colourDefault);
}
