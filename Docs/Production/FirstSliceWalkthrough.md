# 첫 슬라이스 작업 해설

> 현재 상태: 씬 흐름과 입력/캐릭터 기본감 검증은 완료됐습니다. 아래 입력 설정과 테스트 절차는 초기 구축 과정을 설명하는 기록이며 남은 작업 목록이 아닙니다. 현재 작업 위치와 다음 작업은 `VerticalSliceChecklist.md`만 기준으로 판단합니다.

이번 문서는 무엇을 먼저 만들었고, 왜 그렇게 했고, 언리얼 에디터에서 네가 직접 무엇을 눌러 확인하거나 이어서 작업해야 하는지 설명합니다.

## 자동으로 추가한 것

### 1. 프로젝트 방향 문서

파일:

- `README.md`
- `Docs/Design/GameDirection.md`
- `Docs/Design/ControlsAndInput.md`
- `Docs/Production/VerticalSlicePlan.md`

이 문서들은 단순한 소개문이 아니라 제작 기준입니다. 나중에 기능 아이디어가 생기면 다음 질문에 대답해봅니다.

- 이 기능이 근접전 중심성을 강화하는가?
- 신성한 계율과 금단 기술의 대비에 도움이 되는가?
- 첫 버티컬 슬라이스에 필요한가?
- 패드 조작을 불편하게 만들지는 않는가?

아니라면 나중으로 미룹니다.

### 2. 기본 입력 기준

파일과 에셋:

- `Content/00_Core/Input/IMC_Gameplay`
- `Content/00_Core/Input/IA_*`
- `Config/DefaultInput.ini`

실제 키 배치는 `IMC_Gameplay`에서만 관리합니다. `DefaultInput.ini`는 Enhanced Input 클래스를 쓰기 위한 프로젝트 설정만 남깁니다.

이렇게 해야 방어가 LT인지 LB인지, 액티브 조합이 어디에 묶였는지 같은 문제가 한 곳에서만 결정됩니다.

### 3. 캐릭터 입력 처리

파일:

- `Source/Zorba/Public/Player/ZorbaCharacter.h`
- `Source/Zorba/Private/Player/ZorbaCharacter.cpp`

C++ 캐릭터는 이제 `PrimaryAttack`, `Defend`, `Dodge`, `DarkForm`, `UseAbilitySlot1` 같은 입력 이름을 받습니다.

이동, 카메라, 달리기 토글, 회피, 어둠의 형상, 금단 기술 레이어는 기본 동작이 있습니다. 공격과 금단 기술은 아직 실제 판정이 아니라 로그와 Blueprint 이벤트로 연결되어 있습니다.

예시 흐름:

```text
플레이어가 좌클릭을 누른다
IMC_Gameplay에서 좌클릭이 IA_PrimaryAttack으로 매핑되어 있다
ZorbaCharacter가 RequestPrimaryAttack을 받는다
C++에서 "Primary attack requested" 로그를 남긴다
나중에 Blueprint 이벤트 OnPrimaryAttackRequested에서 공격 애니메이션을 재생할 수 있다
```

### 4. 일시정지 입력

파일:

- `Source/Zorba/Public/Player/ZorbaPlayerController.h`
- `Source/Zorba/Private/Player/ZorbaPlayerController.cpp`

`IA_Pause` 입력은 현재 `BP_ZorbaCharacter`에 연결하고, 실제 일시정지 처리는 `ZorbaPlayerController`가 맡습니다. 캐릭터는 입력 에셋을 받아 컨트롤러에 요청만 전달하고, 컨트롤러는 메뉴나 일시정지 같은 플레이어 단위 행동을 처리합니다.

## 왜 이 순서인가

첫 버티컬 슬라이스는 전투를 만들기 전에 플레이어 행동의 이름을 고정해야 합니다.

나쁜 순서:

```text
공격을 만든다
금단 기술을 만든다
UI를 만든다
나중에 패드 조작이 불편하다는 걸 깨닫는다
전부 다시 고친다
```

좋은 순서:

```text
플레이어 의도 이름을 정한다
입력을 한 번 묶는다
간단한 동작을 붙인다
전투, UI, 애니메이션, 튜토리얼이 그 이름을 따라가게 한다
```

그래서 적, 타이틀 UI, 아트보다 입력을 먼저 만들었습니다.

## 초기 구축 당시 에디터 확인 절차

### 0단계 - C++ 빌드 전에는 에디터를 닫기

언리얼 에디터가 열려 있으면 `Binaries/Win64/UnrealEditor-Zorba.dll` 파일을 잡고 있습니다.

Visual Studio나 명령줄에서 다시 빌드하기 전에는:

1. 언리얼에서 열린 맵과 에셋을 저장합니다.
2. 언리얼 에디터를 닫습니다.
3. 빌드합니다.
4. 다시 `.uproject`를 엽니다.

이걸 잊으면 코드가 맞아도 링크 단계에서 실패할 수 있습니다.

### 1단계 - 게임 모드 확인

프로젝트는 이미 `Config/DefaultEngine.ini`에 기본 게임 모드를 지정해두었습니다.

```text
GlobalDefaultGameMode=/Script/Zorba.ZorbaGameMode
```

에디터에서 확인하는 방법:

1. 언리얼 에디터를 엽니다.
2. **Edit > Project Settings**로 갑니다.
3. 검색창에 **Maps & Modes**를 입력합니다.
4. **Default GameMode**를 확인합니다.
5. `ZorbaGameMode` 또는 그 블루프린트 자식을 가리키면 됩니다.

지금은 C++ `ZorbaGameMode`만으로 충분합니다.

### 2단계 - 나중에 캐릭터 블루프린트 만들기

C++ 클래스는 행동의 기반입니다. 블루프린트 자식은 메시와 애니메이션 같은 표현을 붙이는 곳입니다.

1. **Content Drawer**를 엽니다.
2. `Content/00_Core/Blueprints` 폴더를 만듭니다.
3. 폴더 안에서 우클릭합니다.
4. **Blueprint Class**를 선택합니다.
5. **All Classes**를 펼칩니다.
6. `ZorbaCharacter`를 검색합니다.
7. 선택합니다.
8. 이름을 `BP_ZorbaCharacter`로 정합니다.

자동으로 있는 것:

- 이동 컴포넌트
- 카메라 붐
- 팔로우 카메라
- 외형이 없을 때 방향을 확인하기 위한 임시 박스 표식
- 입력 함수 훅
- 달리기 토글 속도 값
- 회피 임시 동작
- 어둠의 형상 임시 동작

네가 채워야 하는 것:

- 스켈레탈 메시
- 애니메이션 블루프린트
- 공격 애니메이션
- 회피 애니메이션
- 발소리
- 전투 판정 트레이스 또는 히트박스

### 3단계 - 나중에 게임 모드가 블루프린트 캐릭터를 쓰게 하기

`BP_ZorbaCharacter`를 만든 뒤:

1. `Content/00_Core/Blueprints/BP_ZorbaGameMode`를 만듭니다.
2. 부모 클래스로 `ZorbaGameMode`를 선택합니다.
3. `BP_ZorbaGameMode`를 엽니다.
4. **Default Pawn Class**를 `BP_ZorbaCharacter`로 설정합니다.
5. **Edit > Project Settings > Maps & Modes**로 갑니다.
6. **Default GameMode**를 `BP_ZorbaGameMode`로 설정합니다.

C++ 게임 모드는 기본 규칙을 정합니다. 블루프린트 게임 모드는 메시나 캐릭터 블루프린트를 바꾸기 쉽게 해줍니다.

### 4단계 - 현재 입력 테스트

1. 부트 맵이나 테스트 맵을 엽니다.
2. **Play**를 누릅니다.
3. WASD와 마우스 시점을 확인합니다.
4. Space로 회피를 확인합니다.
5. Left Ctrl로 어둠의 형상을 확인합니다.
6. 이동 중 Shift를 눌러 달리기가 켜지고, 다시 눌러 꺼지는지 확인합니다.
7. 이동을 멈추면 달리기가 자동으로 꺼지는지 확인합니다.
8. Shift를 누른 상태로 이동을 시작하면 바로 달리는지 확인합니다.
9. 좌클릭, 우클릭, 마우스 휠 클릭, 1, 2, 3, 4, Q, E, F, T를 눌러봅니다.
10. **Window > Output Log**를 엽니다.
11. `Primary attack requested` 같은 로그가 찍히는지 봅니다.

패드 테스트:

1. Play를 누르기 전에 패드를 연결합니다.
2. 왼쪽 스틱 이동과 오른쪽 스틱 카메라를 확인합니다.
3. RT, RB, LB, LT, A, B, X, Y, D-pad 위, D-pad 아래를 눌러봅니다.
4. LT를 누른 채 A/B/X/Y를 눌렀을 때 `Ability slot 1/2/3/4 requested.` 로그가 찍히는지 확인합니다.

LT + 얼굴 버튼 금단 기술 조합은 현재 C++에서 금단 기술 레이어 상태를 기억한 뒤 기존 얼굴 버튼 입력을 금단 기술 슬롯으로 돌리는 방식입니다. 그래서 Enhanced Input의 Chorded Action을 쓰지 않아도 먼저 테스트할 수 있습니다.

## 에셋은 어디에 둘까

Unity의 `Resources` 같은 임시 폴더를 만들기보다, 처음부터 의미 있는 폴더에 둡니다.

추천 구조:

```text
Content/00_Core/Input
Content/00_Core/Blueprints
Content/00_Core/UI
Content/10_Player
Content/20_World/Maps
Content/30_Enemies
Content/40_VFX
Content/50_Audio
Content/80_Dev/TestMaps
```

언리얼은 Unity식 `Resources` 폴더를 쓰지 않습니다. 처음에는 블루프린트에서 직접 참조하는 방식으로 충분합니다.

데이터 기반 미션 콘텐츠는 이미 `Content/00_Core/Data` 아래의 `ZorbaMissionDefinition`을 스캔하도록 설정되어 있습니다.

Addressables 같은 시스템을 지금 도입할 필요는 없습니다. 언리얼에서는 Asset Manager, Soft Reference, Primary Asset이 그 역할을 하지만, 첫 슬라이스에서는 이미 있는 미션 데이터 정도에만 사용합니다.

## 타이틀 그림과 커서 아이콘

이번 단계에서는 타이틀 그림이나 커서 아이콘이 필요하지 않습니다.

이유:

- 첫 증명 대상은 비주얼이 아니라 조작감과 전투 루프입니다.
- 지금 타이틀 이미지를 만들면 실제 게임성을 막지 않는 장식부터 관리하게 됩니다.
- 타이틀과 일시정지 메뉴 레이아웃이 잡힌 뒤 UI 아트를 넣는 편이 좋습니다.

나중에 타이틀 UI를 만들 때 임시 에셋은 아래에 둡니다.

```text
Content/00_Core/UI
```
