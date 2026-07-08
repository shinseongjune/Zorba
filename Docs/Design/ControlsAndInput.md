# 조작과 입력

## 설계 원칙

Zorba의 조작은 **패드 우선**으로 설계하고, 그 다음 키보드/마우스로 옮깁니다.

키보드/마우스는 버튼 수가 많아 기능이 늘어도 흡수하기 쉽습니다. 반대로 패드는 초반에 편하지 않으면 게임 구조 자체가 불편해집니다.

입력의 기준은 하나로 둡니다.

- 물리 키 배치: `IMC_Gameplay` Enhanced Input 에셋
- 입력 액션 에셋: `IA_*`
- 행동 처리: C++ `ZorbaCharacter`
- `DefaultInput.ini`: Enhanced Input을 쓰기 위한 엔진 설정만 유지

즉, 키를 바꾸거나 프리셋을 만들 때는 `DefaultInput.ini`나 C++에 키 이름을 추가하지 않고 `IMC_Gameplay`와 이후 입력 설정 UI를 통해 바꿉니다.

## 전투 액션 모델

Zorba는 "기술을 하나씩 넘긴 뒤 사용 버튼을 누르는 방식"이 아니라, **기술 슬롯 4개를 직접 누르는 방식**을 기본으로 합니다.

플레이어는 캐릭터별 고정 기술 4개를 가집니다.

- 기술 슬롯 1
- 기술 슬롯 2
- 기술 슬롯 3
- 기술 슬롯 4

기술은 이야기 진행에 따라 해금합니다. 첫 버전에서는 넓은 빌드 조합을 목표로 하지 않습니다.

## 패드 기본안

| 의도 | 입력 |
| --- | --- |
| 이동 | 왼쪽 스틱 |
| 카메라 | 오른쪽 스틱 |
| 기본 공격 | RT / R2 |
| 강공격 | RB / R1 |
| 방어 / 패리 | LB / L1 |
| 기술 레이어 | LT / L2 누르고 있기 |
| 기술 1 | LT + A / Cross |
| 기술 2 | LT + B / Circle |
| 기술 3 | LT + X / Square |
| 기술 4 | LT + Y / Triangle |
| 회피 | A / Cross |
| 불경한 대쉬 | B / Circle |
| 상호작용 / 처형 | X / Square |
| 캐릭터 액션 / 태세 | Y / Triangle |
| 성유물 / 회복 | D-pad 아래 |
| 목표 표시 | D-pad 위 |
| 달리기 토글 | 왼쪽 스틱 클릭 |
| 카메라 리셋 | 오른쪽 스틱 클릭 |
| 일시정지 | Menu / Options |

현재 C++ 입력 처리는 `LT / L2`를 기술 레이어로 기억합니다. 그래서 `LT`를 누른 상태에서 A/B/X/Y를 누르면 기존 회피/대쉬/상호작용/캐릭터 액션 대신 기술 슬롯 1/2/3/4가 요청됩니다.

달리기는 이동 중에만 유효합니다. 이동 중 스프린트 입력을 누르면 토글되고, 이동을 멈추면 자동으로 꺼집니다. 스프린트 입력을 누른 상태로 이동을 시작해도 달리기로 들어갑니다.

## 키보드/마우스 기본안

| 의도 | 입력 |
| --- | --- |
| 이동 | WASD |
| 카메라 | 마우스 |
| 기본 공격 | 좌클릭 |
| 강공격 | 마우스 4번 버튼 |
| 방어 / 패리 | 우클릭 |
| 회피 | Space |
| 불경한 대쉬 | Left Ctrl |
| 달리기 토글 | 왼쪽 Shift |
| 상호작용 / 처형 | E |
| 캐릭터 액션 / 태세 | Q |
| 기술 1 | 1 |
| 기술 2 | 2 |
| 기술 3 | 3 |
| 기술 4 | 4 |
| 성유물 / 회복 | F |
| 목표 표시 | T |
| 카메라 리셋 | 마우스 휠 클릭 |
| 일시정지 | Esc |

## UI 기본안

| 의도 | 키보드/마우스 | 패드 |
| --- | --- | --- |
| 포커스 이동 | WASD / 방향키 / 마우스 | D-pad / 왼쪽 스틱 |
| 확인 | Enter / 좌클릭 | A / Cross |
| 뒤로가기 | Esc / 우클릭 | B / Circle |
| 이전 탭 | Q | LB / L1 |
| 다음 탭 | E | RB / R1 |
| 값 초기화 | R | Y / Triangle |
| 스크롤 | 마우스 휠 | 오른쪽 스틱 |

첫 UI는 복잡한 가상 커서 화면보다 포커스 이동이 명확한 메뉴를 우선합니다. 가상 커서는 나중에 복잡한 화면이 필요해질 때 보조로 추가합니다.

## 첫 입력 액션 이름

코드와 에셋이 공유할 플레이어 의도 이름입니다.

```text
Move
Look
PrimaryAttack
HeavyAttack
Defend
Dodge
ProfaneDash
Sprint
AbilityLayer
ContextAction
ClassAction
UseAbilitySlot1
UseAbilitySlot2
UseAbilitySlot3
UseAbilitySlot4
UseRelic
ShowObjective
CameraReset
Pause
UIConfirm
UIBack
UINavigate
UITabNext
UITabPrev
UIReset
```

첫 슬라이스에서는 미루는 입력입니다.

```text
LockTarget
Reload
SwitchWeapon
UseEquipment
Ping
Chat
Scoreboard
```
