#include "Sandbox.hpp"
#include <ELSys/Timer.hpp>
#include <ELSys/Utilities.hpp>

void Sandbox::_Init()
{
	_gl.CreateDummyAndUse();
	GL::LoadDummyExtensions();
	_window.Create("Neural Net Sandbox");
	_gl.Delete();
	_gl.Create(_window);
	_gl.Use(_window);
	GL::LoadExtensions(_window);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	FT_Error error = FT_Init_FreeType(&_ftLib);
	if (error) { Debug::Error("Freetype init error"); }

	_unlit.Load("Data/Shaders/Unlit.vert", "Data/Shaders/Unlit.frag");

	_fontManager.Initialise();
	_textureManager.Initialise();
	_materialManager.Initialise(_textureManager);
	_meshManager.Initialise();

	_context.Set(&_ftLib, &_gl, &_fontManager, &_textureManager, &_materialManager, &_meshManager);

	_fontManager.SetRootPath("Data/Fonts/");
	_fontManager.AddPath(Utilities::GetSystemFontDir());
	_textureManager.SetRootPath("Data/Textures/");

	_uiNodeMat.SetDiffuse(_textureManager.Get("circle1024", _context));
	_uiNodeMatPtr = SharedPointer<const Material>(_uiNodeMat);
	
	_uiFont = _fontManager.Get("consolas", _context);
	
	_camera.projection.SetType(EProjectionType::ORTHOGRAPHIC);
	_camera.projection.SetNearFar(-10.f, 10.f);
	_camera.projection.SetOrthographicScale(1.f);

	_CreateNode(0.f, 0.f);
	_CreateNode(256.f, 0.f);
}

void Sandbox::_Cleanup()
{
	FT_Done_FreeType(_ftLib);
}

bool Sandbox::_Loop()
{
	WindowEvent e;
	Timer timer;
	while (true)
	{
		timer.Start();

		while (_window.PollEvent(e))
		{
			switch (e.type)
			{
			case WindowEvent::CLOSED:
				return true;
				
			case WindowEvent::RESIZE:
				_camera.projection.SetDimensions(Vector2T(e.data.resize.w, e.data.resize.h));
				glViewport(0, 0, e.data.resize.w, e.data.resize.h);
				_Render();
				break;

			case WindowEvent::LEFTMOUSEDOWN:
				_MouseDown();
				break;

			case WindowEvent::LEFTMOUSEUP:
				_MouseUp();
				break;

			case WindowEvent::RIGHTMOUSEDOWN:
				_KeyDown(EKeycode::MOUSE_RIGHT);
				break;

			case WindowEvent::RIGHTMOUSEUP:
				_KeyUp(EKeycode::MOUSE_RIGHT);
				break;

			case WindowEvent::SCROLLWHEEL:
				if (e.data.scrollWheel.lines < 0)
					_KeyDown(EKeycode::MOUSE_SCROLLDOWN);
				else if (e.data.scrollWheel.lines > 0)
					_KeyDown(EKeycode::MOUSE_SCROLLUP);
				break;

			case WindowEvent::MOUSEMOVE:
				_MouseMove(e.data.mouseMove.x, e.data.mouseMove.y);
				break;
			}
		}

		_Frame();
		_lastFrameTime = timer.SecondsSinceStart();
	}
}

void Sandbox::_Frame()
{
	

	_Render();
}

void Sandbox::_Render()
{
	_uiQueue.Clear();
	
	RenderEntry& e = _uiQueue.CreateEntry(ERenderChannels::UNLIT, -1);
	e.AddSetTexture(RCMDSetTexture::Type::WHITE, 0);
	e.AddSetColour(Colour(.125f, .125f, .125f));
	e.AddSetLineWidth(2.f);
	e.AddGrid(_camera.transform, _camera.projection, EAxis::Z, 64.f, 0.f, 0.f, 9.f);

	if (_connection.start)
	{
		e.AddSetColour(Colour::White);
		e.AddSetLineWidth(3.f);
		const AbsoluteBounds& absBounds = _connection.start->GetAbsoluteBounds();
		e.AddLine(Vector3(absBounds.x + absBounds.w / 2.f, absBounds.y + absBounds.h / 2.f), Vector3(_mX, _mY));
	}

	_ui.Render(_uiQueue);

	//Draw
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_LEQUAL);

	_unlit.Use();
	_unlit.SetMatrix4(DefaultUniformVars::mat4Projection, _camera.projection.GetMatrix());
	_unlit.SetMatrix4(DefaultUniformVars::mat4View, _camera.transform.GetInverseTransformationMatrix());
	_uiQueue.Render(ERenderChannels::UNLIT, &_meshManager, &_textureManager, 0);

	_window.SwapBuffers();
}

void Sandbox::_MouseUp()
{
	_ui.KeyUp(EKeycode::MOUSE_LEFT);
	_connection.start.Clear(); //any connections will have been handled by the ui callbacks

	_UpdateNet();
}

void Sandbox::_MouseDown()
{
	if (!_ui.KeyDown(EKeycode::MOUSE_LEFT))
	{
		UINode* node = _CreateNode(_mX, _mY);
		node->OnMouseMove(_mX, _mY, false);
		node->KeyDown(EKeycode::MOUSE_LEFT);
	}
}

void Sandbox::_MouseMove(uint16 x, uint16 y)
{
	_mX = x - _camera.projection.GetDimensions().x / 2.f;
	_mY = _camera.projection.GetDimensions().y / 2.f - y;

	_ui.OnMouseMove(false, _mX, _mY);
}

void Sandbox::_KeyUp(EKeycode key)
{
	_ui.KeyUp(key);

	if (key == EKeycode::MOUSE_RIGHT)
		_UpdateNet();
}

void Sandbox::_KeyDown(EKeycode key)
{
	_ui.KeyDown(key);

	if (key == EKeycode::MOUSE_SCROLLDOWN || key == EKeycode::MOUSE_SCROLLUP)
		_UpdateNet();
}

UINode* Sandbox::_CreateNode(float x, float y)
{
	UINode* node = new UINode(&_ui);
	node->SetMaterial(_uiNodeMatPtr);
	node->SetFont(_uiFont);
	node->SetBounds(UIBounds(UICoord(0.f, x - 64.f), UICoord(0.f, y - 64.f), UICoord(0.f, 128.f), UICoord(0.f, 128.f)));
	node->SetOnEdgeMouseDown(FunctionPointer(this, &Sandbox::_StartConnection));
	node->SetOnHoverMouseUp(FunctionPointer(this, &Sandbox::_EndConnection));
	node->SetOnDeleteRequest(FunctionPointer(this, &Sandbox::_DeleteNode));
	node->perceptron = _pTracker.Track(&*_perceptrons.Emplace());

	_p2n[node->perceptron.Ptr()] = node;

	return node;
}

void Sandbox::_DeleteNode(UINode& node)
{
	_p2n[node.perceptron.Ptr()] = nullptr;
	_uiNodeTracker.Null(&node);
	_pTracker.Null(node.perceptron.Ptr());
	delete &node;
}

void Sandbox::_StartConnection(UINode& from)
{
	_connection.start = _uiNodeTracker.Track(&from);
}

void Sandbox::_EndConnection(UINode& to)
{
	if (_connection.start && to.perceptron && to.perceptron->GetInput(_connection.start->perceptron.Ptr()) == nullptr)
	{
		UIConnection* connection = new UIConnection(&_ui);
		connection->SetTexture(_textureManager.Get("Arrow1024", _context));
		connection->SetFont(_uiFont);
		connection->SetFrom(_connection.start);
		connection->SetTo(_uiNodeTracker.Track(&to));
		connection->SetOnRightClick(FunctionPointer(this, &Sandbox::_DeleteConnection));
	}
}

void Sandbox::_DeleteConnection(UIConnection& connection)
{
	if (connection.GetTo()->perceptron)
		connection.GetTo()->perceptron->RemoveInput(connection.GetFrom()->perceptron.Ptr());

	delete &connection;
}

void Sandbox::_UpdateNet()
{
	Buffer<UIElement*> children = _ui.GetChildren();
	for (UIElement* e : children)
	{
		UIConnection* connection = dynamic_cast<UIConnection*>(e);
		if (connection)
		{
			const UINode* from = connection->GetFrom().Ptr();
			const UINode* to = connection->GetTo().Ptr();

			if (!from || !to || from == to)
			{
				if (to)
					to->perceptron->ClearInputs();

				// The destructor removes the element from its container's children. This is why I'm using a copied buffer.
				delete connection;
				continue;
			}

			float o = to->GetAbsoluteBounds().x - from->GetAbsoluteBounds().x;

			if (o == 0.f)
			{
				Debug::Error("Perfectly vertical lines are confusing!!");
				return;
			}

			if (o < 0.f)
			{
				Utilities::Swap(from, to);
			}

			from->perceptron->RemoveInput(to->perceptron.Ptr());

			NetPerceptron::InputPair* i = to->perceptron->GetInput(from->perceptron.Ptr());

			if (i)
			{
				if (i->weight != connection->GetWeight())
				{
					i->weight = connection->GetWeight();
					to->perceptron->Invalidate();
				}
			}
			else
				to->perceptron->AddInput(from->perceptron.Ptr(), connection->GetWeight());
		}
	}

	for (NetPerceptron& p : _perceptrons)
		p.UpdateValidity();

	for (UIElement* e : _ui.GetChildren())
	{
		UINode* node = dynamic_cast<UINode*>(e);
		if (node)
		{
			bool state = node->perceptron->Evaluate();
			node->SetColourDefault(state ? UIColour(Colour::Green, Colour::Grey) : UIColour(Colour::Red, Colour::Grey));
		}
	}
}


int Sandbox::Run()
{
	_Init();

	_window.Show();

	bool success = _Loop();
	_Cleanup();

	return success ? 0 : 1;
}
