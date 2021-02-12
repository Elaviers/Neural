#include "UIConnection.hpp"
#include <ELGraphics/RenderQueue.hpp>

bool UIConnection::_PointOverlapsLine(const Vector2& point) const
{
	if (_from && _to)
	{
		AbsoluteBounds bFrom = _from->GetAbsoluteBounds();
		AbsoluteBounds bTo = _to->GetAbsoluteBounds();
		Vector2 from = Vector2(bFrom.x + bFrom.w / 2.f, bFrom.y + bFrom.h / 2.f);
		Vector2 to = Vector2(bTo.x + bTo.w / 2.f, bTo.y + bTo.h / 2.f);
		Vector2 axis = to - from;
		float dist = axis.Length();
		axis /= dist; //Normalise
		float dot = axis.Dot(point - from);
		Vector2 closestPoint;
		if (dot <= 0.f)
		{
			closestPoint = from;
		}
		else if (dot >= dist)
		{
			closestPoint = to;
		}
		else
		{
			closestPoint = from + axis * dot;
		}

		float testRadius = _lineWidth + 5.f;
		return (closestPoint - point).LengthSquared() <= (testRadius * testRadius);
	}

	return false;
}

bool UIConnection::_PointOverlapsCentre(const Vector2& point) const
{
	if (_from && _to)
	{
		AbsoluteBounds bFrom = _from->GetAbsoluteBounds();
		AbsoluteBounds bTo = _to->GetAbsoluteBounds();
		Vector2 from = Vector2(bFrom.x + bFrom.w / 2.f, bFrom.y + bFrom.h / 2.f);
		Vector2 to = Vector2(bTo.x + bTo.w / 2.f, bTo.y + bTo.h / 2.f);

		return (point - (from + to) / 2.f).LengthSquared() <= (_size * _size) / 4;
	}

	return false;
}

bool UIConnection::OverlapsPoint(float x, float y) const
{
	Vector2 v(x, y);
	if (_PointOverlapsCentre(v) || _PointOverlapsLine(v))
		return true;

	return false;
}

void UIConnection::Render(RenderQueue& queue) const
{
	if (_from && _to)
	{
		AbsoluteBounds bFrom = _from->GetAbsoluteBounds();
		AbsoluteBounds bTo = _to->GetAbsoluteBounds();
		Vector2 from = Vector2(bFrom.x + bFrom.w / 2.f, bFrom.y + bFrom.h / 2.f);
		Vector2 to = Vector2(bTo.x + bTo.w / 2.f, bTo.y + bTo.h / 2.f);
		Vector2 centre = (from + to) / 2.f;
		Vector2 axis = to - from;
		float dist = axis.Length();
		axis /= dist;

		float angle = Maths::RadiansToDegrees(Maths::ArcTan2(-axis.y, axis.x));

		if (angle > 90.f)
			angle -= 180.f;
		else if (angle < -90.f)
			angle += 180.f;

		Transform t = Transform(Vector3(centre.x, centre.y, 0.f), Vector3(0.f, 0.f, angle), Vector3(dist, _lineWidth, 1.f));

		RenderEntry& e = queue.CreateEntry(ERenderChannels::UNLIT, -1);
		e.AddSetTexture(RCMDSetTexture::Type::WHITE, 0);
		e.AddSetColour(_hover ? _lineColourHover : _lineColour);
		e.AddSetTransform(t.GetTransformationMatrix());
		e.AddCommand(RCMDRenderMesh::PLANE);

		e.AddSetTexture(*_texture, 0);
		(_centreHover ? _centreColourHover : _centreColour).Apply(e);
		t.SetScale(Vector3(_size, _size, 1.f));
		e.AddSetTransform(t.GetTransformationMatrix());
		e.AddCommand(RCMDRenderMesh::PLANE);

		if (_font)
		{
			float fontSize = _size / 2.f;
			String str(String::FromFloat(_weight));
			_fontColour.Apply(e);
			t.SetScale(Vector3(fontSize, fontSize, 1.f));
			t.Move(Vector3(fontSize / -2.f, fontSize / -2.f, 0.f));
			_font->RenderString(e, str.GetData(), t);
		}
	}
}

bool UIConnection::OnKeyUp(bool blocked, EKeycode key)
{
	if (_centreHover && key == EKeycode::MOUSE_RIGHT)
	{
		_onRightClick(*this);
		return true;
	}

	return false;
}

bool UIConnection::OnKeyDown(bool blocked, EKeycode key)
{
	if (_centreHover)
	{
		switch (key)
		{
		case EKeycode::MOUSE_SCROLLDOWN:
			--_weight;
			return true;
		case EKeycode::MOUSE_SCROLLUP:
			++_weight;
			return true;
		}

	}

	return false;
}

bool UIConnection::OnMouseMove(bool blocked, float mouseX, float mouseY)
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

	Vector2 v(mouseX, mouseY);
	bool hoverCentre = _PointOverlapsCentre(v);
	bool hoverLine = hoverCentre ? true : _PointOverlapsLine(v);
	bool hover = hoverCentre || hoverLine;
	if (_hover != hover || _centreHover != hoverCentre)
	{
		_hover = hover;
		_centreHover = hoverCentre;
		_hover ? OnHoverStart() : OnHoverStop();
	}

	return _hover;
}

void UIConnection::OnHoverStart()
{

}

void UIConnection::OnHoverStop()
{

}
