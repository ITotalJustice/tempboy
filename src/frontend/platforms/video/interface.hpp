#pragma once

#include "frontend/types.hpp"

#include <string>
#include <vector>
#include <deque>
#include <functional>



namespace mgb::platform::video {

// this header tries to be a generic interface for
// platform video / window systems.

// windows systems should handle input, display, errors etc...

enum class RenderType {
	SOFTWARE,
	ACCELERATED,
	ACCELERATED_VSYNC,
};

struct VideoInfo {
	std::string name;
	RenderType render_type;
	int x, y, w, h;
};

struct GameTextureInfo {
	int x, y, w, h;
};

struct GameTextureData {
	const uint16_t* pixels;
	int w, h;
};

struct Callbacks {
	// checks to see if all functions are set!
	auto Validate() const {
		if (!this->OnFileDrop) {
			return false;
		}
		if (!this->OnAction) {
			return false;
		}
		if (!this->OnQuit) {
			return false;
		}
		return true;
	}

	// this is called when a file is dragged onto the
	// display window.
	std::function<bool(std::string path)> OnFileDrop;
	std::function<void(Action action, bool down)> OnAction;
	std::function<void()> OnQuit;
};

class Interface {
public:
	Interface(Callbacks _cb) : callback{_cb} {}
	virtual ~Interface() = default;
	
	auto RenderDisplay() -> void;
	
	auto SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool;

	virtual auto UpdateGameTexture(GameTextureData data) -> void = 0;

	virtual auto PollEvents() -> void = 0;

	virtual auto ToggleFullscreen() -> void = 0;
	
	virtual auto SetWindowName(const std::string& name) -> void = 0;

	auto PushTextPopup(const std::string& text) -> void;

	auto ClearTextPopups() -> void;

	auto GetTextPopupCount() const -> std::size_t;

protected:
	class TextPopup final {
	public:
		enum class TickResult { OK, POPME };

	public:
		TextPopup(std::string _text);
		auto GetText() const -> const std::string&;
		auto Tick() -> TickResult;

	private:
		static constexpr auto COUNTER_MAX = 60 * 4; // 4 seconds

		std::string text;
		size_t counter = 0; // ticked until COUNTER_MAX
	};

protected:
	virtual auto RenderDisplayInternal() -> void = 0;
	virtual auto SetupVideoInternal(VideoInfo vid_info, GameTextureInfo game_info) -> bool = 0;

protected:
	Callbacks callback;

	std::vector<std::uint16_t> game_pixels;
	
	std::deque<TextPopup> text_popups;

	bool is_video_setup = false;

private:

};


} // namespace mgb::platform::video
