# Enhanced Input 설정 순서

이 문서는 패드 입력을 살리기 위해 에디터에서 어떤 Input Action과 Mapping Context를 만들어야 하는지 설명합니다.

## 먼저 알아둘 것

이 프로젝트의 gameplay 입력 기준은 **Input Action**과 **Input Mapping Context** 에셋입니다.

`DefaultInput.ini`에는 Enhanced Input을 쓰기 위한 엔진 설정만 남깁니다. 이동, 공격, 회피, 패드 버튼 같은 실제 키 배치는 `IMC_Gameplay`에서만 관리합니다.

현재 프로젝트의 런타임 경로는 `BP_ZorbaGameMode -> BP_ZorbaCharacter -> IMC_Gameplay/IA_*`입니다. 키와 버튼 배치는 `IMC_Gameplay`만 고치고, `DefaultInput.ini`나 C++에 같은 키 배치를 다시 만들지 않습니다.

## 1단계 - 입력 폴더 만들기

1. 언리얼 에디터를 엽니다.
2. 아래쪽 **Content Drawer**를 엽니다.
3. `Content/00_Core` 폴더가 없다면 만듭니다.
4. 그 안에 `Input` 폴더를 만듭니다.

최종 경로:

```text
Content/00_Core/Input
```

## 2단계 - Input Action 만들기

`Content/00_Core/Input` 폴더 안에서 우클릭합니다.

```text
Input > Input Action
```

다음 에셋을 만듭니다.

| 이름 | Value Type |
| --- | --- |
| `IA_Move` | Axis2D |
| `IA_Look` | Axis2D |
| `IA_LookRate` | Axis2D |
| `IA_PrimaryAttack` | Boolean |
| `IA_HeavyAttack` | Boolean |
| `IA_Defend` | Boolean |
| `IA_Dodge` | Boolean |
| `IA_DarkForm` | Boolean |
| `IA_Sprint` | Boolean |
| `IA_AbilityLayer` | Boolean |
| `IA_ContextAction` | Boolean |
| `IA_ClassAction` | Boolean |
| `IA_UseAbilitySlot1` | Boolean |
| `IA_UseAbilitySlot2` | Boolean |
| `IA_UseAbilitySlot3` | Boolean |
| `IA_UseAbilitySlot4` | Boolean |
| `IA_UseRelic` | Boolean |
| `IA_ShowObjective` | Boolean |
| `IA_Pause` | Boolean |

`IA_Look`은 마우스용입니다. `IA_LookRate`는 패드 오른쪽 스틱용입니다. 둘을 나누면 마우스와 패드 감도를 따로 다루기 쉽습니다.

`IA_Pause`는 에셋을 연 뒤 Details에서 **Trigger When Paused**를 켭니다. 그래야 일시정지 상태에서도 같은 입력으로 다시 해제할 수 있습니다.

카메라 리셋 액션은 현재 입력 세트에서 제외합니다. 마우스 휠 클릭은 강공격에 사용합니다.

## 3단계 - Mapping Context 만들기

같은 폴더에서 우클릭합니다.

```text
Input > Input Mapping Context
```

이름:

```text
IMC_Gameplay
```

`IMC_Gameplay`를 열고 위에서 만든 Input Action들을 추가합니다.

## 4단계 - 키보드/마우스 매핑

`IMC_Gameplay` 안에서 다음처럼 키를 넣습니다.

| Input Action | 키 |
| --- | --- |
| `IA_Move` | W, A, S, D |
| `IA_Look` | Mouse XY 2D-Axis, Y-axis Negate |
| `IA_PrimaryAttack` | Left Mouse Button |
| `IA_HeavyAttack` | Middle Mouse Button |
| `IA_Defend` | Right Mouse Button |
| `IA_Dodge` | Space Bar |
| `IA_DarkForm` | Left Ctrl |
| `IA_Sprint` | Left Shift |
| `IA_ContextAction` | E |
| `IA_ClassAction` | Q |
| `IA_UseAbilitySlot1` | 1 |
| `IA_UseAbilitySlot2` | 2 |
| `IA_UseAbilitySlot3` | 3 |
| `IA_UseAbilitySlot4` | 4 |
| `IA_UseRelic` | F |
| `IA_ShowObjective` | T |
| `IA_Pause` | Escape |

`IA_Move`에서 WASD를 2D로 만들 때는 보통 다음처럼 잡습니다.

```text
W = Y +1
S = Y -1
D = X +1
A = X -1
```

에디터에서 키를 추가한 뒤 각 키의 Modifier에 **Swizzle Input Axis Values**나 **Negate**를 넣어 방향을 맞춥니다. 이 부분은 에디터 UI에서 조금 헷갈릴 수 있으니, 먼저 W/S만 넣고 이동이 되는지 확인한 다음 A/D를 맞추는 식으로 해도 됩니다.

## 5단계 - 패드 매핑

패드는 다음처럼 넣습니다.

| Input Action | 패드 |
| --- | --- |
| `IA_Move` | Gamepad Left 2D-Axis |
| `IA_LookRate` | Gamepad Right 2D-Axis |
| `IA_PrimaryAttack` | Gamepad Right Trigger |
| `IA_HeavyAttack` | Gamepad Right Shoulder |
| `IA_Defend` | Gamepad Left Shoulder |
| `IA_Dodge` | Gamepad Face Button Bottom |
| `IA_DarkForm` | Gamepad Face Button Right |
| `IA_Sprint` | Gamepad Left Thumbstick |
| `IA_AbilityLayer` | Gamepad Left Trigger, Gamepad Left Trigger Axis |
| `IA_ContextAction` | Gamepad Face Button Left |
| `IA_ClassAction` | Gamepad Face Button Top |
| `IA_UseRelic` | Gamepad D-pad Down |
| `IA_ShowObjective` | Gamepad D-pad Up |
| `IA_Pause` | Gamepad Special Right |

금단 기술 4개는 최종적으로 `LT + A/B/X/Y` 조합으로 갑니다. 어둠의 형상은 그 4슬롯과 별도이며, `B / Circle` 자체에 남습니다. 현재 C++에서는 `IA_AbilityLayer`가 눌린 상태를 기억한 뒤, A/B/X/Y 기본 액션을 금단 기술 슬롯으로 돌려보내는 방식으로 처리합니다.

스프린트 규칙:

```text
이동 중이 아닐 때 Sprint 입력만 누르면 아직 달리지 않음
Sprint를 누른 상태로 이동을 시작하면 달림
이동 중 Sprint를 누르면 달리기 on/off 토글
이동 입력이 끝나면 달리기는 자동 off
```

## 5-1단계 - LT + 얼굴 버튼 금단 기술 조합 만들기

액티브 레이어용 Input Action은 이미 위 표에 포함되어 있습니다.

| 이름 | Value Type |
| --- | --- |
| `IA_AbilityLayer` | Boolean |

`IMC_Gameplay`에 다음 매핑을 추가합니다.

| Input Action | 패드 |
| --- | --- |
| `IA_AbilityLayer` | Gamepad Left Trigger, Gamepad Left Trigger Axis |

그 다음에는 얼굴 버튼을 따로 금단 기술 슬롯에 매핑하지 않습니다. A/B/X/Y는 기존 기본 액션에 그대로 둡니다.

현재 코드의 처리 방식:

```text
LT / L2 누름 = IA_AbilityLayer 시작
A / Cross 입력 = 원래는 Dodge
하지만 AbilityLayer가 눌려 있으면 Dodge 대신 Ability Slot 1 요청

B / Circle 입력 = 원래는 DarkForm
하지만 AbilityLayer가 눌려 있으면 DarkForm 대신 Ability Slot 2 요청
X / Square 입력 = Ability Slot 3
Y / Triangle 입력 = Ability Slot 4
```

이 방식은 Enhanced Input의 Chorded Action 우선순위 옵션에 덜 의존합니다. 초반 제작에서는 이쪽이 더 안정적입니다.

주의:

- `IA_AbilityLayer`는 반드시 `BP_ZorbaCharacter`의 `Zorba|Input`에 연결해야 합니다.
- `IA_UseAbilitySlot1~4`는 반드시 `BP_ZorbaCharacter`의 `Zorba|Input`에 연결해야 합니다.
- 패드에서는 `IA_UseAbilitySlot1~4`에 얼굴 버튼을 추가하지 않는 편이 좋습니다. 키보드의 숫자 1~4용으로만 써도 됩니다.
- 만약 예전에 `IA_UseAbilitySlot1~4`에 Chorded Action을 넣었다면, 일단 그 패드 매핑은 지우고 테스트하세요.

디자인상 `IA_UseAbilitySlot1~4`는 전부 금단 기술 슬롯입니다. 신성한 계율은 방어, 패리, 회피, 강공격 같은 기본 공방 입력 위에 붙습니다.

## 6단계 - BP_ZorbaCharacter 만들기

아직 없다면 만듭니다.

1. `Content/00_Core/Blueprints` 폴더를 만듭니다.
2. 우클릭합니다.
3. **Blueprint Class**를 선택합니다.
4. **All Classes**를 열고 `ZorbaCharacter`를 검색합니다.
5. 선택해서 `BP_ZorbaCharacter`를 만듭니다.

## 7단계 - Input Action 연결하기

`BP_ZorbaCharacter`를 엽니다.

왼쪽 또는 오른쪽의 **Details** 패널에서 `Zorba|Input` 카테고리를 찾습니다.

다음처럼 연결합니다.

| 속성 | 넣을 에셋 |
| --- | --- |
| `Default Mapping Context` | `IMC_Gameplay` |
| `Move Action` | `IA_Move` |
| `Look Action` | `IA_Look` |
| `Look Rate Action` | `IA_LookRate` |
| `Primary Attack Action` | `IA_PrimaryAttack` |
| `Heavy Attack Action` | `IA_HeavyAttack` |
| `Defend Action` | `IA_Defend` |
| `Dodge Action` | `IA_Dodge` |
| `Dark Form Action` | `IA_DarkForm` |
| `Sprint Action` | `IA_Sprint` |
| `Ability Layer Action` | `IA_AbilityLayer` |
| `Context Action Input` | `IA_ContextAction` |
| `Class Action Input` | `IA_ClassAction` |
| `Use Ability Slot 1 Action` | `IA_UseAbilitySlot1` |
| `Use Ability Slot 2 Action` | `IA_UseAbilitySlot2` |
| `Use Ability Slot 3 Action` | `IA_UseAbilitySlot3` |
| `Use Ability Slot 4 Action` | `IA_UseAbilitySlot4` |
| `Use Relic Action` | `IA_UseRelic` |
| `Show Objective Action` | `IA_ShowObjective` |
| `Pause Action` | `IA_Pause` |

저장하고 컴파일합니다.

## 8단계 - GameMode가 BP_ZorbaCharacter를 쓰게 하기

현재 프로젝트에는 `BP_ZorbaGameMode`가 있으며, `Config/DefaultEngine.ini`의 `GlobalDefaultGameMode`도 이 블루프린트를 가리킵니다.

1. `Content/00_Core/Blueprints`에서 우클릭합니다.
2. **Blueprint Class**를 선택합니다.
3. 부모 클래스로 `ZorbaGameMode`를 선택합니다.
4. 이름을 `BP_ZorbaGameMode`로 합니다.
5. `BP_ZorbaGameMode`를 열고 **Default Pawn Class**를 `BP_ZorbaCharacter`로 설정합니다.
6. **Edit > Project Settings > Maps & Modes**로 갑니다.
7. **Default GameMode**를 `BP_ZorbaGameMode`로 설정합니다.

이제 Play를 누르면 C++ 기본 캐릭터가 아니라 입력 에셋이 연결된 `BP_ZorbaCharacter`가 나와야 합니다.

## 9단계 - 패드가 그래도 안 될 때

Enhanced Input 에셋을 만들었는데도 패드가 안 되면 입력 매핑 문제가 아니라 **패드가 Windows/Unreal에 제대로 인식되지 않는 문제**일 수 있습니다.

특히 DualShock 계열은 Windows에서 Xbox 패드처럼 바로 잡히지 않는 경우가 많습니다.

확인 순서:

1. Windows에서 `게임 컨트롤러 설정`을 엽니다.
2. 패드 입력이 실제로 들어오는지 확인합니다.
3. Unreal Editor를 켜기 전에 패드를 먼저 연결합니다.
4. Xbox 패드가 아니라면 DS4Windows나 Steam Input처럼 XInput으로 변환해주는 방법을 고려합니다.

나중에 DualShock을 정식으로 직접 지원하고 싶다면 Raw Input 플러그인이나 플랫폼별 입력 처리를 따로 검토합니다. 첫 슬라이스에서는 패드를 Xbox 입력처럼 인식시키는 편이 가장 빠릅니다.
