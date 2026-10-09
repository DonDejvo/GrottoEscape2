#pragma once

#include <glm/vec2.hpp>
#include <string>
#include <vector>
#include <functional>
#include <array>
#include "constants.h"

class GameState;

class UIElement {
public:
    UIElement();
    glm::vec2 position;
    virtual void Update(GameState& game) = 0;
};

struct MenuItem {
    std::string label;
    bool isCheckbox = false;
    struct {
        bool checked = false;
        std::string checkedText;
        std::string uncheckedText;
    } checkbox;
    std::function<void(GameState& game, bool checked)> onClick = nullptr;

    void Click(GameState& game, bool left = false);
};

class Menu : public UIElement {
public:
    Menu();
    void Update(GameState& game) override;
    void ResetIndex();
    void AddItem(MenuItem&& item);
    inline const std::vector<MenuItem>& getItems() const { return mItems; }
    inline int getActiveIndex() const { return mActiveIdx; }

private:
    std::vector<MenuItem> mItems;
    int mActiveIdx;
};

class PasswordInput : public UIElement {
public:
    std::string message;

    PasswordInput();
    void SetPassword(const std::string& password);
    void Update(GameState& game) override;
    std::string GetValue() const;
    inline int getActiveIndex() const { return mActiveIdx; }
    inline bool IsInvalid() const { return mIsInvalid; }

private:
    int mActiveIdx;
    double mCounter;
    std::array<int, LEVEL_PASSWORD_LENGTH> mValue;
    bool mIsInvalid;

    static int getCharIndex(char ch);
};

class Dialog : public UIElement {
public:
    enum class Type {
        System,
        Help
    };

    std::string text;
    bool isConfirm;
    bool defaultValue;
    std::function<void(GameState& game)> onConfirm;
    Type type;

    Dialog(Type type, const std::string& text, bool isConfirm, bool defaultValue = false, std::function<void(GameState& game)> onConfirm = nullptr);
    Dialog();
    void Update(GameState& game) override;
    int GetAnimationFrame() const;
    inline bool IsOpened() const { return mOpened; }
    inline void SetOpened(bool opened) { mOpened = opened; }
    inline bool GetValue() const { return mValue; }

private:
    bool mOpened;
    bool mValue;
    double mAnimCounter;
};