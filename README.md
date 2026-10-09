# Grotto Escape II

2D platformer game written in C++. Work in progress!

## Controls

| Control name | Mobile | PC |
|---|---|---|
| **Menu** | | |
| Navigation | D-Pad | WASD |
| Enter selected item | B | K / Enter |
| Exit password menu | Start | Backspace |
| **Level** | | |
| Pause / Resume | Start | Backspace |
| Switch player | L / R | Left / Right control |
| Movement | D-Pad | WASD |
| Primary ability | B | K / Enter |
| Secondary ability | Y | J |
| Interact | A | L |
| **Inventory** | | |
| Use item | X | I |
| Select item | Select + D-Pad | Tab + WASD |
| Transfer item | B (on selected item) + D-Pad | K (on selected item) + WASD |

## Players

### Ralph

Ralph is player number one who wears yellow suit.

#### Jump (primary)

Ralph can jump only when touching the ground. The jumping height increases depending on the movement speed at the moment of the jump off.

#### Wall break (secondary)

If there is breakable wall right in front of Ralph when activates this ability, it breaks. Ralph can't move while using this ability.

### Gregory

Gregory is player number two who wears blue suit.

#### Levitation (primary)

Gregory can switch to levitation mode if he is not climbing ladder nor using other ability. While in the levitation mode Gregory's falling speed is decreased but his movement acceleration and deceleration is also decreased.

#### Plasma bullet (secondary)

Gregory can shoot plasma bullet while touching the ground and not in the levitation mode. The bullet deals 1 damage or can activate interactable objects by hit. Gregory can't move while using this ability.

## Level passwords

- LVL1
- LVL2