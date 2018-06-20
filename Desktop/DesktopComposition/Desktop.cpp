#include "pch.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;

template <typename T>
struct DesktopWindow
{
	static T* GetThisFromHandle(HWND const window) noexcept
	{
		return reinterpret_cast<T *>(GetWindowLongPtr(window, GWLP_USERDATA));
	}

	static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
	{
		WINRT_ASSERT(window);

		if (WM_NCCREATE == message)
		{
			auto cs = reinterpret_cast<CREATESTRUCT *>(lparam);
			T* that = static_cast<T*>(cs->lpCreateParams);
			WINRT_ASSERT(that);
			WINRT_ASSERT(!that->m_window);
			that->m_window = window;
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));

			EnableNonClientDpiScaling(window);
		}
		else if (T* that = GetThisFromHandle(window))
		{
			return that->MessageHandler(message, wparam, lparam);
		}

		return DefWindowProc(window, message, wparam, lparam);
	}

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
	{
		switch (message) {
		case WM_DPICHANGED:
		{
			return HandleDpiChange(m_window, wparam, lparam);
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		}

		return DefWindowProc(m_window, message, wparam, lparam);
	}

	// DPI Change handler. on WM_DPICHANGE resize the window
	LRESULT HandleDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		HWND hWndStatic = GetWindow(hWnd, GW_CHILD);
		if (hWndStatic != nullptr)
		{
			UINT uDpi = HIWORD(wParam);

			// Resize the window
			auto lprcNewScale = reinterpret_cast<RECT*>(lParam);

			SetWindowPos(hWnd, nullptr, lprcNewScale->left, lprcNewScale->top,
				lprcNewScale->right - lprcNewScale->left, lprcNewScale->bottom - lprcNewScale->top,
				SWP_NOZORDER | SWP_NOACTIVATE);

			if (T* that = GetThisFromHandle(hWnd))
			{
				that->NewScale(uDpi);
			}
		}
		return 0;
	}

	void NewScale(UINT dpi) {

	}

protected:

	using base_type = DesktopWindow<T>;
	HWND m_window = nullptr;
	
};

struct Window : DesktopWindow<Window>
{
	Window() noexcept
	{
		WNDCLASS wc{};
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
		wc.lpszClassName = L"XAML island in Win32";
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		RegisterClass(&wc);
		WINRT_ASSERT(!m_window);

		WINRT_VERIFY(CreateWindow(wc.lpszClassName,
			L"XAML island in Win32",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			nullptr, nullptr, wc.hInstance, this));

		WINRT_ASSERT(m_window);

		InitXaml(m_window, m_rootGrid, m_scale);

		//TODO: extract this into a createcontent function
		//TODO: hook up resizing
		Windows::UI::Xaml::Controls::Button b;
		b.Width(200);
		b.Height(30);
		b.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
			box_value(Windows::UI::Xaml::VerticalAlignment::Top));

		Windows::UI::Xaml::Controls::TextBlock tb;
		tb.Text(L"Hello Win32 love XAML xx");
		b.Content(tb);

		m_rootGrid.Children().Append(b);
	}

	void InitXaml(
		HWND wind,
		Windows::UI::Xaml::Controls::Grid & root,
		Windows::UI::Xaml::Media::ScaleTransform & dpiScale) {

		Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

		static DesktopWindowXamlSource source;
		auto interop = source.as<IDesktopWindowXamlSourceNative>();
		check_hresult(interop->AttachToWindow(wind));
		HWND h = nullptr;
		interop->get_WindowHandle(&h);
		SetWindowPos(h, 0, 0, 0, 600, 300, SWP_SHOWWINDOW);

		Windows::UI::Xaml::Media::ScaleTransform st;
		dpiScale = st;

		//TODO: get correct DPI here
		dpiScale.ScaleX(3.0);
		dpiScale.ScaleY(3.0);

		//TODO: Background color
		Windows::UI::Xaml::Controls::Grid g;
		g.RenderTransform(st);
		g.Width(600);
		g.Height(300);

		Windows::UI::Xaml::Media::SolidColorBrush background{ Windows::UI::Colors::Red() };
		g.Background(background);

		root = g;
		source.Content(g);
	}

	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
	{
		// TODO: handle messages here...
		return base_type::MessageHandler(message, wparam, lparam);
	}

	void NewScale(UINT dpi) {

		auto scaleFactor = (float)dpi / 100;

		m_scale.ScaleX(scaleFactor);
		m_scale.ScaleY(scaleFactor);
	}


private:

	DesktopWindowTarget m_target{ nullptr };
	Windows::UI::Xaml::Media::ScaleTransform m_scale{ nullptr };
	Windows::UI::Xaml::Controls::Grid m_rootGrid{ nullptr };
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	init_apartment(apartment_type::single_threaded);

	Window window;
	
	MSG message;

	while (GetMessage(&message, nullptr, 0, 0))
	{
		DispatchMessage(&message);
	}
}
