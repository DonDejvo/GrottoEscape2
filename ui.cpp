#include "ui.h"

#include "engine.h"

const int DIALOG_ANIM_FRAME_DURATION = 250;
const std::string LEVEL_PASSWORD_CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!";
const int PASSWORD_INPUT_PRESS_DURATION = 200;

UIElement::UIElement()
	: position(0)
{
}

void MenuItem::Click(GameState& game, bool left)
{
	if (isCheckbox) {
		checkbox.checked = !checkbox.checked;
	}
	if (onClick) onClick(game, checkbox.checked);
}

Menu::Menu()
	: mActiveIdx(0)
{
}

void Menu::Update(GameState& game) {
	const Input& input = Engine::GetInput();

	if (input.IsClicked(Input::DPAD_UP)) {
		mActiveIdx = (mItems.size() + mActiveIdx - 1) % mItems.size();
	}
	else if (input.IsClicked(Input::DPAD_DOWN)) {
		mActiveIdx = (mActiveIdx + 1) % mItems.size();
	}
	else if (input.IsClicked(Input::BUTTON_B) || 
		(mItems[mActiveIdx].isCheckbox && (input.IsClicked(Input::DPAD_LEFT) || input.IsClicked(Input::DPAD_RIGHT)))) {
		mItems[mActiveIdx].Click(game, input.IsClicked(Input::DPAD_LEFT));
	}
}

void Menu::ResetIndex()
{
	mActiveIdx = 0;
}

void Menu::AddItem(MenuItem&& item)
{
	mItems.push_back(std::move(item));
}

PasswordInput::PasswordInput()
	: mActiveIdx(0),
	mCounter(0),
	mIsInvalid(false),
	mValue({}),
	message("")
{
}

void PasswordInput::SetPassword(const std::string& password)
{
	for (int i = 0; i < std::min(static_cast<size_t>(LEVEL_PASSWORD_LENGTH), password.size()); ++i) {
		mValue[i] = getCharIndex(password[i]);
	}
}

void PasswordInput::Update(GameState& game)
{
	if (mIsInvalid) {
		mCounter += 1000.0 / 60;
		if (mCounter >= 1500) {
			mIsInvalid = false;
			game.menuState = GameState::MenuState::Main;
			game.mainMenu.ResetIndex();
		}
		return;
	}

	const Input& input = Engine::GetInput();

	if (input.IsClicked(Input::DPAD_UP) || input.IsClicked(Input::DPAD_DOWN)) {
		mCounter = PASSWORD_INPUT_PRESS_DURATION;
	}
	if (input.IsPressed(Input::DPAD_UP) || input.IsPressed(Input::DPAD_DOWN)) {
		mCounter += 1000.0 / 60;

		if (mCounter >= PASSWORD_INPUT_PRESS_DURATION) {
			mCounter = 0;
			if (input.IsPressed(Input::DPAD_UP)) {
				mValue[mActiveIdx] = (mValue[mActiveIdx] + 1) % LEVEL_PASSWORD_CHARSET.size();
			}
			else {
				mValue[mActiveIdx] = (LEVEL_PASSWORD_CHARSET.size() + mValue[mActiveIdx] - 1) % LEVEL_PASSWORD_CHARSET.size();
			}
		}
	}
	else {
		if (input.IsClicked(Input::DPAD_LEFT)) {
			mActiveIdx = (LEVEL_PASSWORD_LENGTH + mActiveIdx - 1) % LEVEL_PASSWORD_LENGTH;
		}
		else if (input.IsClicked(Input::DPAD_RIGHT)) {
			mActiveIdx = (mActiveIdx + 1) % LEVEL_PASSWORD_LENGTH;
		}
		else if (input.IsClicked(Input::BUTTON_B)) {
			if (!game.LoadLevel(GetValue())) {
				mIsInvalid = true;
				mCounter = 0;
			}
		}
		else if (input.IsClicked(Input::BUTTON_START)) {
			game.menuState = GameState::MenuState::Main;
			game.mainMenu.ResetIndex();
		}
	}
}

std::string PasswordInput::GetValue() const
{
	std::string result;
	result.reserve(LEVEL_PASSWORD_LENGTH);
	for (int i = 0; i < LEVEL_PASSWORD_LENGTH; ++i) {
		result += LEVEL_PASSWORD_CHARSET[mValue[i]];
	}
	return result;
}

int PasswordInput::getCharIndex(char ch)
{
	for (int i = 0; i < LEVEL_PASSWORD_CHARSET.size(); ++i) {
		if(LEVEL_PASSWORD_CHARSET[i] == ch) return i;
	}
	return 0;
}

Dialog::Dialog(Type type, const std::string& text, bool isConfirm, bool defaultValue, std::function<void(GameState& game)> onConfirm)
	: type(type),
	text(text),
	isConfirm(isConfirm),
	defaultValue(defaultValue),
	onConfirm(onConfirm),
	mOpened(false),
	mValue(false),
	mAnimCounter(0)
{
}

Dialog::Dialog()
	: Dialog(Type::System, "", false)
{
}

void Dialog::Update(GameState& game)
{
	if (!mOpened) return;

	mAnimCounter += 1000 / 60.0;
	if (mAnimCounter >= DIALOG_ANIM_FRAME_DURATION * 2) {
		mAnimCounter = 0;
	}

	const Input& input = Engine::GetInput();
	if (isConfirm && (input.IsClicked(Input::DPAD_LEFT) || input.IsClicked(Input::DPAD_RIGHT))) {
		mValue = input.IsClicked(Input::DPAD_LEFT);
		mAnimCounter = 0;
	}
	else if (input.IsClicked(Input::BUTTON_B)) {
		bool value = mValue;
		mOpened = false;
		mValue = defaultValue;
		if (value) {
			onConfirm(game);
		}
	}
	else if (input.IsClicked(Input::BUTTON_START) || (type != Type::System && input.IsClicked(Input::BUTTON_A))) {
		mOpened = false;
		mValue = defaultValue;
	}
}

int Dialog::GetAnimationFrame() const
{
	return mAnimCounter < DIALOG_ANIM_FRAME_DURATION ? 0 : 1;
}
