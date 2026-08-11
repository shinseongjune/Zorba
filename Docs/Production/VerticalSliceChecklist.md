# 현재 제작 체크리스트

이 파일만 현재 작업 순서와 다음 재개 지점을 관리합니다. 날짜별 완료 이력은 누적하지 않고, 아래 `현재 작업 인계`를 작업 종료 때마다 최신 상태로 덮어씁니다. 다음 작업은 반드시 이 인계를 먼저 읽고 시작하며, 이미 승인된 증거를 별도 단계로 다시 열지 않습니다.

## 현재 작업 인계 — 2026-08-11 갱신

사용자가 광폭화와 금단 기술 해제를 PIE에서 정상 동작으로 승인하고 전투 게이트 종료를 지시했으며, `L_Boot → M01 → 성공/실패 → 재시작/부트 복귀` 전체 흐름도 정상 동작으로 승인했습니다. 따라서 기존 전투 항목과 M01 1인 미션 루프는 회귀가 생기지 않는 한 다시 열지 않습니다. **현재 활성 게이트는 M01 버티컬 슬라이스 폴리시입니다.** 다음 재개 지점은 현재 그레이박스에 간단한 목표 안내와 남은 적 가독성, 시작→조우→결과의 공간 동선, 최소 결과 연출을 더한 뒤 한 라운드 전체 플레이테스트를 수행하는 것입니다. 전용 모션과 SFX/VFX, AI Perception·NavMesh·Behavior Tree/StateTree, 최종 캐릭터 교체는 기능 계약을 유지한 채 후속 표현·AI 확장·최종 통합 게이트에서 처리합니다.

### 완료·실증

- `DA_Player_Light01.Montage`가 유일한 재생 소스입니다. `BP_ZorbaCharacter`의 하드코딩 `OnPrimaryAttackRequested → Play Anim Montage` 노드를 삭제했고 BP 컴파일 `Good to go`와 저장을 확인했습니다.
- 공격 시작 방향 저장·회전, Montage 전체 재입력/방향 잠금, `AttackActive`/`HitCommit` Notify 수신, 정상 종료·중단 정리를 `UZorbaMeleeCombatComponent`가 소유합니다.
- `ForwardArc`가 대상 선택을 맡고 활성 구간의 `Trace_Base`/`Trace_Tip` Sweep은 접촉 정보만 보정합니다. Player `0`/Enemy `1`, 사망·아군 제외, 공격별 동일 대상 1회 규칙을 적용했습니다.
- 재사용 가능한 `BP_ZorbaEnemyBase`를 만들었습니다. Team `1`, ASC, Health/Stamina, 피격 밀림, 사망·스태미나 고갈 상태 전환을 가지며 `L_OpenWorldTestArena`의 `GoldenHit_Enemy`로 배치했습니다.
- `HitCommit` 수신 프레임에 무기 Trace를 즉시 샘플링하고 마지막 접점을 타격 표현 위치로 보존합니다. Montage 인스턴스가 사라지거나 AnimInstance가 교체돼도 공격 상태를 정리하는 감시 경로를 추가했습니다.
- MaxHealth/MaxCombatStamina가 현재 값보다 낮아질 때 현재 값도 함께 낮춰 `현재값 ≤ 최대값` 계약을 유지합니다.
- UE 5.8 정식 `ZorbaEditor Win64 Development` 빌드가 통과했습니다. 허용 목록의 `BP_ZorbaCharacter`와 `BP_ZorbaEnemyBase`만 다시 컴파일해 오류 `0`, 경고 `0`을 확인했습니다.
- 에셋 검증에서 플레이어 공격 Data Asset 참조, Montage, HitPhase, Range `220`, HalfAngle `55`, MaxTargets `3`, 테스트 맵 로드를 확인했습니다. World Partition 외부 액터 파일에도 `GoldenHit_Enemy`와 `BP_ZorbaEnemyBase_C`가 존재합니다.
- UE 자동화 `Project.Maps.PIE`가 `L_OpenWorldTestArena`를 Map Check 오류 `0`/경고 `0`으로 열고 Standalone PIE를 5초 실행·정상 종료했습니다. GameplayCue 검색 경로를 `/Game/60_FX`로 제한한 뒤 테스트 이벤트 경고도 `0`입니다.
- 최종 Standalone PIE 첫 공격은 `WeaponTrace` 접촉으로 Health `100 → 90`, Stamina `100 → 95`, 후보 `1`, 적용 `1`이었습니다.
- 빠른 더블클릭 공격은 시작·피해가 한 번만 발생했고 `BroadCoverageFallback`으로 Health `90 → 80`, Stamina `95 → 90`, 후보 `1`, 적용 `1`이었습니다. 공격 종료 뒤 다음 공격도 정상 복구됐습니다.
- 사용자 PIE에서 W/A/S/D·대각선·무입력 공격 방향, 공격 중 방향 고정, 범위·각도 안팎 판정, `DA_Player_Light01.Range` 변경 반영을 승인했습니다. 이 증거는 다시 열지 않습니다.
- `AZorbaCharacter::Move()`가 `IsAttackInProgress()` 동안 이동 입력을 무시하고, 공격 시작 성공 시 해당 프레임에 누적된 이동 입력과 현재 속도 및 스프린트 상태를 정리하도록 구현했습니다. 별도 잠금 플래그를 추가하지 않아 Montage 종료로 공격 상태가 정리되면 다음 이동 입력부터 자동 복구됩니다.
- UE 5.8 `ZorbaEditor Win64 Development` 재빌드가 성공했습니다. 이어서 자동화 `Project.Maps.PIE`가 현재 시작 맵 `L_Boot`를 열어 `Success`, 종료 코드 `0`으로 끝났습니다. 이 자동화는 프로젝트 기동 회귀 증거이며 실제 이동 입력 차단의 체감 증거는 아닙니다.
- 사용자 PIE에서 이동 중 공격 시 즉시 정지·Montage 동안 이동 차단과, 이동 키를 계속 누른 상태에서 Montage 종료 직후 자동 이동 복구를 승인했습니다.
- `UZorbaEnemyBasicAttackComponent`가 플레이어 Team `0`을 거리로 선택하고, `DA_Enemy_Basic01`과 공용 `UZorbaMeleeCombatComponent`를 통해 공격 재생·Notify 판정·2.5초 재공격 주기를 수행합니다. 테스트 맵 자동 실행에서 매 공격마다 Health `10`, Stamina `5`가 정확히 한 번만 감소했습니다.
- 방어는 전방 ±75도 안에서 유지되며, 시작 후 `0.22초`만 `State.ParryWindow`, 이후에는 `State.Defending`만 남습니다. 자동 실행에서 적 기본 공격 Health `10 → 2`, Stamina `5 → 5`로 감소하고 `Defense=Blocked`를 확인했습니다.
- 패리는 플레이어 피해를 `0`으로 만들고 적 공격 Montage를 강제 중단한 뒤 적 Stamina를 `35` 감소시키며 `0.5초` 피격 경직을 적용합니다. 자동 실행에서 플레이어 Health/Stamina `100/100` 유지, 적 Stamina `100 → 65 → 30 → 0`, 세 번째 패리 뒤 후속 공격 중단을 확인했습니다.
- `BP_ZorbaCharacter.DefendAction`은 `IA_Defend`, 기본 컨텍스트는 `IMC_Gameplay`로 지정돼 있습니다. 기준 입력은 키보드/마우스 우클릭, 패드 LB/L1입니다.
- 위 변경 뒤 UE 5.8 정식 `ZorbaEditor Win64 Development` 빌드가 성공했습니다. 방어·패리 자동 실행은 Shipping에서 제외되는 명시적 테스트 옵션으로 상태만 준비하며, 공격과 피해는 제품 코드·Data Asset·Montage Notify 경로를 그대로 사용합니다.
- 사용자 PIE에서 방어 피해 감소, 패리 무피해·적 공격 중단, 방어 입력 해제 뒤 일반 피격 복귀를 모두 승인했습니다. 방어·패리 기능 게이트는 여기서 닫습니다.
- 단독 강공격 `DA_Player_Heavy01`은 스태미나 비용 `20`, 체력/자세 피해 `20/40`, 약→강 파생 `DA_Player_HeavyAfterLight01`은 비용 `15`, 피해 `25/50`으로 분리했습니다. 기본 공격의 `AttackActive` 종료가 파생 입력창을 열고, 파생 시작 시 기존 Montage를 정리한 뒤 저장된 공격 방향을 그대로 승계합니다.
- 강공격 자동 실행에서 플레이어 스태미나 `100 → 80`, 적 Health/Stamina `100/100 → 80/60`을 확인했습니다. 약→강 자동 실행은 기본 공격 뒤 적 `90/95`, 파생 적중 뒤 `65/45`, 플레이어 스태미나 `100 → 85`로 끝나 두 공격 모두 한 번만 적용됐습니다.
- 적 Stamina가 `0`이면 `State.Exhausted`로 6초간 이동·공격이 막히며, 기본 공격 입력이 유효한 탈진 적을 우선 선택해 `DA_Player_Opportunity01`을 실행합니다. 기회공격 동안 공격자는 제한 무적이며, 적에게 체력 피해 `35`를 한 번 적용한 뒤 탈진을 소비하고 Stamina `50`을 복구합니다.
- 처형은 상호작용 입력에서 체력 비율 `25%` 이하 적을 우선 선택합니다. `DA_Player_Execution01`이 공격자/피격자 쌍 Montage, 정렬 거리와 회복량을 소유하며, 적을 확정 사망시킨 뒤 플레이어 Health `30`, Combat Stamina `50`을 회복합니다.
- 기회공격 자동 실행은 `Enemy exhausted → PlayerOpportunity01 → Health 100 → 65 → Exhausted consumed` 순서를, 처형 자동 실행은 `PlayerExecution01 → Health 20 → 0 → Enemy executed → Health 30/Stamina 50 recovery` 순서를 실제 제품 경로에서 확인했습니다.
- 네 공격은 모두 `UZorbaAttackDefinition`의 Montage를 유일한 재생 소스로 사용합니다. C++/Blueprint에서 별도 공격 Montage를 재생하지 않으므로 나중에 클립을 바꿀 때 Data Asset의 Montage와 `AttackActive`/`HitCommit` 위치만 교체하면 판정·피해·자원·상태 로직은 유지됩니다.
- 사용자 PIE에서 단독 강공격 뒤 강공격을 다시 누르면 약→강 파생이 잘못 재생되는 회귀를 발견했습니다. 원인은 `DA_Player_Light01` 복제 시 `bOpenHeavyBranchAfterHitWindow=true`가 강공격·파생·기회·처형 Data Asset에도 복사된 것이었습니다.
- 기본공격만 파생창 플래그를 갖도록 네 비기본 공격 에셋을 교정했고, 런타임도 현재 공격이 `PrimaryAttackDefinition`이자 `Standard`일 때만 `DerivedHeavy` 전환을 허용하도록 이중 방어했습니다. 자동 회귀 실행은 강공격 진행 중 재입력을 무시하고 종료 뒤 `PlayerHeavy01`을 다시 시작했으며 `PlayerHeavyAfterLight01`은 한 번도 시작하지 않았습니다. 별도 약→강 자동 실행에서는 파생이 기존처럼 정상 시작됐습니다.
- 사용자 PIE에서 강공격 진행 중 강공격 재입력 무시와 종료 뒤 단독 강공격 재시작을 확인했습니다. 강공격 파생 회귀 게이트는 여기서 닫습니다.
- 적에 `Fodder/Elite/Boss` 전투 등급을 추가하고 기존 `BP_ZorbaEnemyBase`는 정예로 유지했습니다. 파생 `BP_ZorbaEnemyFodder`는 Health `30`, Combat Stamina `35`, 크기 `0.88`이며 임시 머리 위 역할 라벨로 같은 대역 모델을 구분합니다.
- 졸개 패리는 공격을 중단한 뒤 치명 피해를 같은 사망 경로에 넣는 즉결처형입니다. `DA_Player_Execution01`의 Health `30`·Combat Stamina `50` 회복과 `InstantExecutionInvulnerabilityDuration=1.25초` 제한 무적을 발동하며, 쌍 Montage·카메라 고정은 사용하지 않습니다. `OnParryInstantKill` Blueprint 이벤트에 나중에 짧은 전용 모션만 추가할 수 있습니다. 정예는 기존 패리 스태미나 피해→`Exhausted`→기회공격/처형 흐름을 유지합니다.
- `UZorbaEnemyCrowdSubsystem`이 적별 안정된 포위 슬롯과 대상별 근접 공격 토큰만 관리합니다. 공격 상한은 `1`이고 대기 적은 플레이어 주위 반경 `340`의 각자 슬롯으로 이동합니다. 피격·패리·탈진·사망은 계속 각 적 액터가 독립적으로 소유합니다.
- 테스트 맵에 졸개 3명과 기존 정예 1명을 묶는 `CrowdEncounter_01`을 배치했습니다. 졸개 패리 제품 경로 자동 실행에서 조우 `Enemies=4`, 졸개 Health `30 → 0`, 플레이어 Health/Stamina `40/25 → 70/75`, 즉결처형 무적 `1.25초` 시작·종료를 확인했습니다. 무적 중 뒤에서 이어진 다른 졸개의 `EnemyBasic01`은 대상에서 제외됐고, 공격권 상한도 계속 `max Active=1`을 유지했습니다.
- 군중 배치 뒤 고급 공격 자동 검증이 졸개를 잘못 고르지 않도록 정예를 우선 선택하게 했습니다. 기존 쌍 처형 회귀 실행도 `PlayerExecution01 → 정예 사망 → Health/Stamina 40/25 → 70/75 → Montage 정상 종료` 순서로 다시 통과했습니다.
- 전멸 자동 실행에서 독립 사망 신호가 `Remaining 3 → 2 → 1 → 0`으로 집계되고 `Combat encounter completed`가 정확히 한 번 발생했습니다. 완료 표현은 `OnEncounterClearPresentation` Blueprint 이벤트로 분리해 이후 미션/UI가 같은 신호를 받을 수 있습니다.
- `UZorbaEnemyCombatBrainComponent`를 추가해 대상/상태 판단과 이동 의도를 기존 공격 실행기에서 분리했습니다. 최소 상태는 `Idle → Observe → Approach → Attack → Recover`와 `Defend/Disabled`이며, 관찰 중에는 지켜보기·옆걸음·특수 모션 이벤트 중 하나를 선택합니다. 현재 직접 이동 호출은 이 컴포넌트 한 곳에 모아 후속 NavMesh 이동으로 교체할 수 있게 했습니다.
- `UZorbaEnemyCombatProfile`이 교전·이탈 거리, 관찰 거리/시간/가중치, 공격 간격, 공격별 선택 거리·가중치·재사용 시간·접근 속도, 방어 확률·유지 시간·재사용 시간을 소유합니다. `DA_Enemy_FodderProfile`은 기본 공격 하나와 방어 확률 `0`, `DA_Enemy_EliteProfile`은 기본·패리 유도·회피 전용·장거리 기습 네 공격과 방어 확률 `0.2`를 갖습니다.
- 공격 데이터에 `Standard`, `GuardBreak`, `DodgeOnly` 방어 상호작용과 `None`, `Parry`, `Dodge`, `Ambush` 표현 신호를 추가했습니다. 플레이어 강공격 두 종류는 `GuardBreak`, `EnemyParry01`은 `GuardBreak+Parry`, `EnemyDodge01`은 `DodgeOnly+Dodge`, `EnemyAmbush01`은 `Standard+Ambush`입니다.
- 적은 전방 유지 방어를 사용할 수 있습니다. 일반 공격은 체력 피해를 `0.2배`로 줄이고, 플레이어 강공격은 방어를 종료한 뒤 전체 피해를 적용합니다. 강공격 자동 실행에서 `Enemy defense started → PlayerHeavy01 → Enemy defense stopped → Health 100→80/Stamina 100→60, Defense=GuardBroken`을 확인했습니다. 적의 회피 전용 공격은 플레이어의 유지 방어와 패리 창을 모두 무시하며 패리 유도 공격은 정확한 패리만 허용합니다.
- 군중 조정자에 일반 공격권과 별개의 대상별 특수 신호권을 추가했습니다. `Parry/Dodge/Ambush` 신호 공격은 접근 시작부터 공격 종료까지 이 권한을 하나만 점유하므로 여러 적이 동시에 서로 다른 특별 대응을 요구하지 않습니다.
- UE 5.8 `ZorbaEditor Win64 Development` 전체 빌드가 새 UHT 타입과 컴포넌트를 포함해 통과했습니다. 에셋 검증은 정예 공격 `4`, 졸개 공격 `1`, 공격별 방어/신호 값, 두 Blueprint의 프로필 참조, 플레이어 강공격 두 종류의 `GuardBreak`를 확인했습니다. 테스트 맵 고정 프레임 실행에서 졸개/정예가 공격권 상한 `1`을 지키며 기본 공격을 반복하고 정예 방어가 주기적으로 시작됐습니다. 장기 실행에서는 `EnemyParry01`과 `EnemyDodge01`이 특수 신호권을 획득·반납했고, 장거리 자동 실행은 정예를 `800cm`에 배치한 뒤 `EnemyAmbush01` 선택→고속 접근→공격 시작→신호권 반납 순서를 통과했습니다.
- 사용자 PIE에서 적 등급·군중 공격권·즉결처형·특수 공격 유형은 정상 동작을 확인했습니다. 다만 정예 가드 `0.9초`는 직접 타격을 시험하기에 짧았고, 관찰 행동이 추격과 지속 옆걸음을 하지 않는 문제가 남아 이 두 항목만 수정 게이트로 다시 열었습니다.
- 관찰 행동과 거리 보정을 분리했습니다. `Watch/SpecialMotion`도 적정 거리 밖에서는 추격·후퇴하고 거리대 안에서만 정지하며, `Strafe`는 매 틱 접선 선행 지점을 갱신해 플레이어를 바라본 채 계속 원을 돕니다. 교전한 표적은 최초 감지 거리를 벗어나도 `LeashRange`까지 추적합니다. 비출시 자동 실행에서 정예가 `800 → 447.6cm`로 접근하고 플레이어 기준 각도를 `33.4°` 바꿔 거리 보정 뒤 측면 이동까지 수행했습니다.
- 정예 Combat Profile의 방어 유지 시간을 `0.9 → 2.25초`로 늘렸습니다. 에셋 검증은 방어 유지 `2.25초`, 거리 허용폭 `75cm`, 옆걸음 선행 거리 `190cm`를 확인했고, 변경 뒤 UE 5.8 `ZorbaEditor Win64 Development` 정식 빌드와 테스트 맵 실행이 통과했습니다.
- 2026-07-29 사용자 PIE 로그에서 적이 `528.8cm`에서 공격 접근을 선택했지만 8초 제한까지 실제 접근을 끝내지 못했고, `197.2cm`와 `200.8cm`처럼 플레이어가 직접 가까이 온 경우에만 공격을 시작한 것을 확인했습니다. 원인은 Brain이 `0.05초`마다 한 번만 Tick하면서 한 프레임만 유지되는 `AddMovementInput`을 사용해, 높은 PIE 프레임레이트에서 대부분의 프레임에 제동이 걸린 것이었습니다.
- Brain 이동 의도를 매 프레임 갱신하도록 바꾼 뒤 UE 5.8 `ZorbaEditor Win64 Development` 정식 빌드가 통과했습니다. 동일한 120FPS 이동 자동화에서 수정 전 `800.0 → 783.1cm`, 이동량 `16.9cm`, 각도 변화 `0.0°`였던 결과가 수정 후 `800.0 → 435.1cm`, 이동량 `1019.0cm`, 각도 변화 `107.5°`로 바뀌어 추격 뒤 지속 옆걸음이 프레임레이트와 무관하게 진행됨을 확인했습니다. 실제 체감과 후퇴는 사용자 PIE 재확인을 유지합니다.
- 사용자 PIE에서 수정된 추격·후퇴·지속 옆걸음과 2.25초 정예 가드를 확인했습니다. 가드 중 기본 공격 피해 감경과 강공격의 가드 브레이크·전체 피해, 졸개가 방어하지 않는 계약까지 승인했으므로 AI 이동·가드 수정 게이트를 닫습니다.
- 첫 신성한 계율을 플레이어 전용 후방 공격 패시브로 구현했습니다. 피격자 후방축 기준 ±60도 안에서 가한 공격은 전방 방어 판정을 건너뛰고 체력 피해만 `1.5배`가 되며 전투 스태미나/자세 피해는 원본을 유지합니다. 무기 Trace가 아니라 두 액터의 방향으로 판정하고 확정 사망 처형은 배율에서 제외합니다.
- UE 5.8 `ZorbaEditor Win64 Development` 정식 빌드가 통과했습니다. 같은 가드 중 정예를 전방에서 기본 공격한 자동 실행은 Health `100 → 98`, Stamina `100 → 95`, `Defense=Blocked`, `SacredDoctrine=None`이었고, 후방 실행은 Health `100 → 85`, Stamina `100 → 95`, `Defense=None`, `SacredDoctrine=RearAttack`이었습니다.
- 공용 피해 경로 회귀 확인에서 전방 강공격은 기존대로 정예 가드를 종료하고 Health/Stamina `100/100 → 80/60`, `Defense=GuardBroken`, `SacredDoctrine=None`을 유지했습니다. 후방 계율 발동은 개발 빌드에서 노란 `SACRED REAR x1.50` 텍스트로 표시합니다.
- 첫 금단 기술이 파훼할 실제 패턴을 만들었습니다. 졸개가 아닌 적은 체력이 최대치의 `50%` 이하가 되는 첫 순간 한 번 `State.Enraged`에 들어갑니다. 광폭 중에는 방어 선택을 멈추고 이동 속도가 `1.35배`, 판단·관찰·공격 쿨다운·회수 간격이 `0.6배`가 됩니다.
- 금단 기술 슬롯 1은 플레이어 전방 `±55도`, 거리 `1000cm` 안의 살아 있는 광폭 적만 대상으로 잡습니다. 성공하면 광폭을 영구 해제하고 `State.Stunned`를 `2.5초` 부여하며, 피해나 전투 스태미나 비용 없이 `8초` 쿨다운을 시작합니다. 잘못된 상태·사거리 밖·비광폭 대상에서는 발동하지 않고 쿨다운도 소비하지 않습니다.
- 적의 `OnEnrageStarted/Ended`, `OnForbiddenTechniqueStunned/StunEnded`와 플레이어의 `OnForbiddenTechniqueSlot1Started/Denied`를 표현·UI 연결 지점으로 분리했습니다. 현재 개발 빌드에서는 적 머리 위 `ENRAGED`/`STUNNED` 역할 라벨과 보라색 방향 화살표로 기능을 확인합니다.
- UE 5.8 `ZorbaEditor Win64 Development` 정식 빌드가 통과했습니다. `L_OpenWorldTestArena` 자동 실행은 비광폭 대상 입력의 쿨다운 `0.00초` 유지, Health `40`에서 광폭 시작, 슬롯 1 직후 `WasEnraged=true → IsEnraged=false`, `IsStunned=true`, 쿨다운 `8.00초`, 즉시 재입력 쿨다운 거절을 기록했습니다. `2.5초` 뒤에는 `IsStunned=false`, `IsEnraged=false`로 이동이 복구됐고 프로세스는 종료 코드 `0`으로 끝났습니다.
- 허용 목록의 `BP_ZorbaCharacter`와 `BP_ZorbaEnemyBase` 제한 컴파일은 오류 `0`, 경고 `0`이었습니다. 이어서 `Project.Maps.PIE`가 현재 시작 맵을 Map Check 오류 `0`/경고 `0`으로 열고 테스트 `Success`, 이벤트 경고 `0`, 종료 코드 `0`으로 끝났습니다. 이 두 자동 검증은 Blueprint 계약과 프로젝트 기동 회귀 증거이며 실제 조작감·식별성 승인은 아닙니다.
- 2026-08-11 사용자가 광폭화와 해제 기술의 정상 동작을 승인하고 전투 게이트 종료를 지시했습니다. 첫 금단 기술 사용자 PIE 항목을 닫았고, 첫 신성한 계율은 새 PIE 증거를 추가했다는 뜻이 아니라 기존 전후방 자동 비교와 전투 게이트 종료 지시에 따라 닫았습니다.
- `/Game/00_Core/Data/DA_Mission_M01`은 `MissionId=M01`, 표시명 `성채 안뜰 소탕`, 맵 `/Game/20_World/Maps/L_M01_Graybox`, 1인 전용으로 저장했습니다. M01에는 `CrowdEncounter_01`과 졸개 3명·정예 1명·PlayerStart가 실제 World Partition 외부 액터로 배치돼 있습니다.
- `AZorbaGameMode`가 `Boot → MissionLoading → MissionActive → MissionComplete/MissionFailed`를 권위 있게 전환합니다. 모든 필수 조우 전멸은 다음 틱에 성공으로 확정하고, 같은 프레임에 플레이어가 사망하면 성공 예약을 취소해 실패를 우선합니다. 결과 단계에는 `RESTART`와 `RETURN TO BOOT`를 제공하며 중복 여행과 잘못된 단계의 요청을 차단합니다.
- 네이티브 최소 미션 UI가 `L_Boot`에서 M01 시작 버튼을, 성공/실패에서 결과와 재시작·부트 복귀 버튼을 표시합니다. `L_OpenWorldTestArena` 같은 비미션 개발 맵은 `Campaign` 샌드박스로 분류해 부트 UI가 기존 전투 입력을 막지 않습니다.
- UE 5.8 정식 `ZorbaEditor Win64 Development` UHT·비유니티 C++·링크 빌드가 성공했습니다. 부트 자동 실행은 `Boot → DA_Mission_M01 로드 → M01 여행 → 적 4 → 0 → MissionComplete`를 종료 코드 `0`으로 마쳤고, 실패 실행은 실제 플레이어 Health `0`과 사망 신호를 거쳐 `MissionFailed`를 기록했습니다. 실패 뒤 재시작은 새 M01의 `MissionActive`, 부트 복귀는 pending mission을 지운 새 `L_Boot`의 `Boot`까지 실제 여행한 뒤 각각 종료 코드 `0`으로 끝났습니다.
- 표준 `Project.Maps.PIE`는 `L_Boot`를 Map Check 오류 `0`/경고 `0`, 테스트 `Success`, 테스트 이벤트 오류·경고 `0`으로 마쳤습니다. M01을 명시해 실행한 같은 테스트도 Map Check 오류 `0`/경고 `0`, 테스트 `Success`, 오류 `0`이었지만 기존 선택형 무기 Trace 마커가 없는 공격의 `Broad attack coverage remains active` 경고가 4회 수집됐습니다. 이 경고는 승인된 넓은 범위 판정 fallback이며 전투 게이트를 다시 열지 않습니다. 자동 증거는 버튼 클릭·화면 배치·체감의 사용자 PIE 승인을 대체하지 않습니다.

### 임시 승인과 남은 증거

- `Attack1`의 길이와 `AttackActive`/`HitCommit` 위치는 기능 검증 수준으로 승인합니다. 사운드·VFX를 붙일 때 보이는 검 접촉 프레임과 한 번만 맞춥니다.
- 검·방패 부착과 `Trace_Base`/`Trace_Tip` 위치는 작동하지만 정렬이 어색합니다. 현재 골든 히트를 막지는 않으며 캐릭터 최종 교체 전 별도 보정합니다.
- 판매자 원본은 `Content/SwordAndShieldAnimationV1`에 있고 공개 저장소에서 제외됩니다. 파일 시스템으로 옮기지 말고 골든 히트 승인 뒤 Content Browser 이동과 Redirector 정리로 `90_ExternalAssets` 규칙을 맞춥니다.
- 패리로 적 Montage를 강제 중단하는 실제 원인이 생겼고, 중단 즉시 공격 상태가 정리되어 다음 재공격 주기로 복구되는 것을 자동 실행에서 확인했습니다. 플레이어 공격의 피격·상태이상 중단은 해당 원인이 구현될 때 별도로 확인합니다.
- 승인된 타격 SFX/VFX가 없으므로 효과 동기화는 현재 기능 게이트를 막지 않습니다. 에셋 도입 뒤 보이는 검 접촉 프레임에 판정·사운드·VFX를 맞추는 표현 게이트로 별도 재개합니다.
- 방어·패리 전용 리타깃 애니메이션과 방패 충돌 SFX/VFX는 아직 없습니다. 현재 판정은 로그와 파란/초록 디버그 표시로 확인하고, 승인 클립과 효과 에셋이 들어올 때 표현 게이트로 묶습니다.
- 모션 연결 지점은 플레이어의 `OnDefendStarted`, `OnDefendStopped`, `OnMeleeHitReceived(DefenseResult)`와 적의 `OnParried` Blueprint 이벤트로 분리했습니다. 나중에 시작·유지·종료·방어 충격·패리 반응 Montage를 붙여도 판정·피해 C++은 수정하지 않습니다.
- 고급 공격용 임시 클립은 단독 강공격 `Attack2_IP`, 약→강 `Attack3_Stage2_Complete_IP`, 기회공격 쌍 `Attack4_Stage2_Complete_IP/React`, 처형 쌍 `Attack10_Stage2_Complete_IP/React`입니다. 플레이어 Garret 대역의 기능 연결은 사용자 승인됐습니다. 공격자/피격자 발 미끄러짐, 방패·검 관통, 접촉 프레임과 거리 `100~105cm`의 최종 품질 보정은 최종 캐릭터 교체와 표현 폴리시 때 다시 확인하며 현재 기능 게이트를 막지 않습니다.
- 판매팩 원본과 `_RetargetTest` 결과는 공개 저장소 제외 상태를 유지합니다. 프로젝트 소유 Montage·Attack Definition만 공개 가능한 산출물이며, 최종 캐릭터로 바꿀 때 같은 Retargeter로 다시 출력하고 기존 Notify 이름을 보존합니다.

### 고정 전투 판정 계약

1. **타격 대상은 공격별 넓은 범위가 결정합니다.** `DA_Player_Light01.HitPhases[0]`의 `Shape`, `Range`, `HalfAngleDegrees`, `HalfHeight`, `MaxTargets`를 저장된 공격 방향에 적용합니다.
2. **무기 Trace는 대상 선정 권한이 없습니다.** `Trace_Base`와 `Trace_Tip` Sweep은 타점·법선·피격 부위·VFX 위치를 보정합니다. 넓은 범위에 든 대상은 얇은 검 Trace가 빗나가도 피해 대상에서 탈락시키지 않습니다.
3. Montage·범위·피해·방향 정책은 `UZorbaAttackDefinition`이 소유합니다. 준비·활성·회수의 실제 프레임 타이밍은 Montage Section/Notify가 소유하며 C++이나 BP에 초 단위로 복제하지 않습니다.
4. `bAttackInProgress`는 Montage 전체의 재입력 차단·방향 잠금을, `bHitWindowOpen`은 `AttackActive` 동안의 접점 수집만 담당합니다.
5. BP와 C++에서 Montage를 이중 재생하지 않습니다. `DA_Player_Light01.Montage`만 재생 소스로 사용합니다.

### 적 AI와 방어 상호작용 계약 — 기능 기반 구현

- Brain은 판단과 이동 의도만, BasicAttackComponent는 선택된 공격 실행만, MeleeCombatComponent는 Montage·Notify·타격 판정만 소유합니다. 완성형 Behavior Tree/StateTree와 AI Perception은 아직 넣지 않습니다.
- 공격별 방어 상호작용은 `UZorbaAttackDefinition` 데이터가 소유합니다. 기본형은 방어·패리 가능, 가드 브레이크형은 유지 방어를 깨지만 정확한 패리는 허용, 회피 전용형은 방어·패리를 모두 무시합니다.
- 역할별 공격 빈도와 선택 거리는 Combat Profile에서 조정합니다. 장거리 기습은 `550~1400cm`에서만 후보가 되고 `1.75배` 접근 속도를 사용합니다. 최종 돌진 이동·충돌·전용 Montage는 후속 모션 게이트입니다.
- 특수 공격은 `OnAttackSignalStarted`, 적 방어는 `OnDefendStarted/Stopped/OnGuardBroken`, 관찰 특수 모션은 `OnObserveActionStarted` Blueprint 이벤트를 제공합니다. 현재 개발 빌드의 임시 색상 텍스트를 최종 GameplayCue/VFX와 Montage로 교체해도 판정 계약은 유지됩니다.
- 직접 이동은 임시 평면 이동입니다. 장애물 회피, NavMesh 경로 탐색, 시야/청각, 귀환 경로, 분대 전술은 실제 전투 공간을 만든 뒤 같은 Brain의 이동/대상 공급부를 교체하는 후속 AI 게이트입니다.
- 관찰 이동은 공격권 유무와 별개로 선호 거리±허용폭을 유지합니다. 너무 멀면 추격하고 너무 가까우면 후퇴하며, 거리대 안에서 `Watch/SpecialMotion`은 지켜보고 `Strafe`는 플레이어를 향한 채 지속 측면 이동합니다. Brain이 회전을 소유하는 동안 이동 방향 자동 회전은 끄고, Brain을 중단하면 기존 설정을 복구합니다.
- 광폭과 금단 기절은 GAS 상태 태그가 공용 계약입니다. Brain과 BasicAttackComponent는 `State.Stunned` 동안 이동·공격을 중단하고, 기절 종료 뒤 죽음·Exhausted·다른 HitReact가 없을 때만 걷기를 복구합니다. 최종 광폭/금단 Montage와 GameplayCue를 붙여도 이 상태 소유권은 바꾸지 않습니다.

### 현재 확인 요청 — M01 버티컬 슬라이스 폴리시

- 시작 화면과 M01 진입 직후에 플레이어가 목표를 즉시 이해할 수 있는 한 줄 안내를 제공합니다.
- PlayerStart에서 조우 공간까지의 이동 방향과 졸개 3명·정예 1명의 우선순위가 그레이박스 실루엣만으로도 읽히도록 동선과 배치를 다듬습니다.
- 전투 중 남은 적 수 또는 진행 상태를 최소 UI로 표시하고, 전멸 성공과 플레이어 사망 실패의 결과 전환을 짧은 연출로 구분합니다.
- 시작→조우→성공/실패→재시작/부트 복귀를 한 라운드로 플레이테스트해 막힘, 길 잃음, 결과 오인 없이 반복 가능한지 승인합니다.

## 제품 에셋 기준

미술 방향은 **현실 비율의 스타일라이즈드 PBR 다크 판타지**로 고정합니다. 인간 군단은 유료 기사 통합팩, 이름 있는 주연과 보스는 소수의 Paragon 제작 베이스, 악마 군단은 하나의 몬스터 통합팩으로 구성합니다. 서로 다른 팩을 그대로 나열하지 않고 공통 문장·색·거칠기·손상·발광 규칙을 다시 적용합니다.

현재 보유한 Polyart `Garret`/`Elara` 두 명은 인간 진영의 최종 베이스에서 제외합니다. 필요하면 원거리 배경 민간인이나 개발용 대역으로만 사용하고, 플레이어·성기사단·조르바의 최종 외형으로 승격하지 않습니다.

### 1. 최종 캐릭터 구성

| 역할 | 최종 베이스 | 애니메이션 소스 |
|---|---|---|
| 플레이어 | `Knights (Pack)`의 경갑 기사 1명을 전용 배정. 낡은 강철, 짧은 타바드, 고유 검집과 얼굴/헬멧으로 일반병과 분리 | `Sword and Shield Animations V1` |
| 성기사 일반병 | 같은 팩의 Templar·Saracen·Medieval Knight 계열 3명. 닫힌 투구, 아이보리/금색, 동일 문장으로 조직화 | `Sword and Shield Animations V1` |
| 성기사 지휘관/타락 기사 | 남은 중갑 기사 또는 무료 Paragon `Greystone` 제작 베이스 | 기사 공용 팩 또는 원본 Paragon AnimBP |
| 조르바(여성 멘토) | 무료 Paragon `Serath` 제작 베이스. 머리·무기·날개·갑옷 외곽선을 교체 | 원본 이동·전투·변신 애니메이션과 FX |
| 비숍/인간 보스 | 기사팩의 Demon Knight 또는 `Greystone`. 성직 문장·관·무기·타바드로 별도 실루엣 제작 | 역할에 맞는 원본/공용 전투 애니메이션 |
| 악마 보병/언데드 | `MONSTERS SERIES BUNDLE`의 Skeleton·Zombies·Mummy | 포함 Undead Animations 및 개별 세트 |
| 악마 플랭커/짐승 | Night Crawler·Giant Spider·Centaur Demon | 포함 Creature Animations 및 개별 세트 |
| 악마 원거리 | Wraith. 전용 투사체와 금단 계열 VFX만 프로젝트에서 연결 | 포함 개별 애니메이션 세트 |
| 악마 브루트 | Golem. 붉은 균열 재질과 성기사단 문장 파편으로 진영화 | 포함 개별 애니메이션 세트 |
| 악마 엘리트 | 무료 Paragon `Grux` 제작 베이스 | 원본 AnimBP·공격 세트 |
| 최종 보스/변신 페이즈 | 무료 Paragon `Sevarog` 제작 베이스 | 원본 AnimBP·애니메이션·FX |

Paragon은 군단 채우기용이 아니라 주연·엘리트·보스 제작 베이스로만 3~4종 사용합니다. 각 캐릭터는 머리/어깨/무기/비율 중 두 곳 이상, 진영 재질·문장·손상 규칙, 원본과 다른 VFX 형태·타이밍을 모두 통과해야 최종 아트로 승인합니다. 색만 바꾼 Paragon 원형은 출시 빌드에 넣지 않습니다.

### 2. 구매 스택

가격은 2026-07-21 Fab Personal 표시가이며 세금은 별도입니다. 결제 시 가격과 라이선스 티어를 다시 확인합니다.

#### 캐릭터와 애니메이션 — 필수

| 역할 | 상품 | 표시가 | 채택 근거 |
|---|---|---:|---|
| 인간 진영 통합 | [Knights (Pack)](https://www.fab.com/listings/6907d3d8-d0db-43c0-bf9c-8ad43c70e7b3) | ₩121,710 | UE 5.8, 고유 캐릭터 5명(남4·여1), Templar·Medieval Knight·Demon Knight 등, 분리 부품·무기·방패·Cloth Physics·UE4/UE5 리그. 단순 2베이스 색놀이가 아니라 플레이어와 병력 실루엣을 분리할 수 있음 |
| 인간 검+방패 전투 | [Sword and Shield Animations V1](https://www.fab.com/listings/c50f1764-0afe-4fce-a338-a25fef492835) | ₩16,210 | UE 5.5~5.8, UE5 마네킹, 287동작, In-place/Root Motion, 공격·방어·패리·피격·사망·회피·동기화 공격 포함 |
| 악마/언데드 통합 | [MONSTERS SERIES BUNDLE](https://www.fab.com/listings/de26c2e1-7e6c-45c3-ae7d-5068d6cda482) | ₩162,290 | 11팩과 Creature/Undead Animation 팩, 수제 LOD. Centaur Demon·Golem·Giant Spider·Mummy·Night Crawler·Skeleton·Wraith·Zombies 8계열만 사용해도 보병·원거리·짐승·브루트가 완성됨. UE 표기는 5.7까지이므로 5.8 스테이징 검증 필수 |
| 조르바 제작 베이스 | [Paragon: Serath](https://www.fab.com/listings/522b6160-15ab-492b-a2b0-c09f9bb5f6e6) | 무료 | UE 5.8, 모델·스킨·애니메이션·FX·AnimBP. 성기사 출신과 빛/어둠 이중성이 조르바의 신성/금단 기술 구조에 맞음 |
| 타락 기사 제작 베이스 | [Paragon: Greystone](https://www.fab.com/listings/122fd7bf-6f12-4304-a930-cccbbacdaebc) | 무료 | UE 5.8, 중갑 기사 실루엣과 검술 세트. 현재 Fab 계정 라이브러리에 등록됨 |
| 악마 엘리트 제작 베이스 | [Paragon: Grux](https://www.fab.com/listings/8c4bac2c-f7f7-4632-a644-47f4e104f5d8) | 무료 | UE 5.8, 중장갑 돌격 엘리트 체형. 현재 Fab 계정 라이브러리에 등록됨 |
| 최종 보스 제작 베이스 | [Paragon: Sevarog](https://www.fab.com/listings/a4882b5e-cfad-4830-a3dd-46a6c31a79b2) | 무료 | UE 5.8, 대형 사신형 실루엣과 전용 애니메이션·FX·AnimBP. 현재 Fab 계정 라이브러리에 등록됨 |

캐릭터·애니메이션 필수 합계는 **₩300,210 + 세금**입니다. 기존 개별 N-Hance 4종보다 모델 수가 훨씬 많고, 유료 보스 구매를 Paragon 제작 베이스로 대체합니다. Fab 계정 등록은 로컬 설치 완료를 뜻하지 않으므로 실제 반입 여부는 별도로 확인합니다.

예산 대안인 [Demon Pack](https://www.fab.com/listings/52c1a06f-17fc-4cec-86ef-8333c44ae226)은 ₩89,250에 Archer·Assassin·Spearman·Berserk·Demon 5병과와 전투 애니메이션을 제공하지만, 큰 머리와 짧은 팔다리의 카툰 비율이라 Paragon/기사팩과 함께 쓰지 않습니다. 게임 전체를 카툰 비율로 바꾸는 결정을 할 때만 통합 악마팩을 이 상품으로 교체하며, 그 경우 캐릭터 합계는 **₩227,170 + 세금**입니다.

순수 악마 화풍을 최우선으로 둘 때의 상위 대안은 [Demons Pack](https://www.fab.com/listings/c9bc76b5-8c38-4a7b-8efd-89e14c6ed158) ₩194,760입니다. Striker·Tank·Executioner·Imp·Warrior 5종과 각 23~35개 애니메이션이 있지만, `MONSTERS SERIES BUNDLE`보다 비싸고 UE 표기가 5.5까지라 현재 승인안에서는 제외합니다.

#### 월드·VFX·오디오 — 제품 완성용

| 범주 | 상품 | 표시가 | 맡길 범위 |
|---|---|---:|---|
| 통합 환경 | [Fantasy Medieval Kingdom](https://www.fab.com/listings/46e9afbc-42d6-4d44-8a8b-a24d547aa95a) | 현재 세일 ₩97,380 | UE 5.8, 600여 메시, 성채·교회·도시·던전·죽은 숲·실내. 데모 오픈월드를 쓰지 않고 3개 미션 구역만 재조립 |
| 마법 VFX | [Niagara Magic VFX Bundle](https://www.fab.com/listings/0b0b18e7-00d7-4e09-b090-479977f6450e) | 현재 세일 ₩32,450 | UE 5.8, 투사체와 광역기. 색/재질을 신성·금단·악마 3계열로 통일 |
| 피격 VFX | [Stylized Blood VFX](https://www.fab.com/listings/42fdcfc8-1346-4bb0-a538-feccb3860b92) | 현재 세일 ₩32,450 | UE 5.8, 베기·찌르기·분출·출혈·상처 데칼. 처형과 일반 피격 강도를 분리 |
| 통합 SFX | [RPG & Fantasy Sounds Bundle](https://www.fab.com/listings/7c7dbd7a-a398-4c35-b192-ae1688b0902e) | ₩73,020 | 1,000 WAV, 무기·방패·마법·괴물·인간 노력음·발소리·환경·문·던전·UI. UE 표기는 5.7까지이므로 WAV를 5.8에서 직접 검증 |
| 음악 | [Dark Fantasy Music Pack](https://www.fab.com/listings/5b2898f8-90e5-463c-be2b-22412edadfba) | ₩8,090 | 38분, 루프·일반곡·스팅어로 탐색/전투/보스 전환 구성. WAV 직접 반입 검증 |

권장 전체 제품 스택은 **₩543,600 + 세금**입니다. 구성은 `캐릭터/애니메이션 ₩300,210 + 월드/VFX ₩162,280 + 오디오 ₩81,110`입니다. 카툰 비율의 Feyloom 악마팩으로 미술 방향 자체를 바꾸면 **₩470,560 + 세금**까지 낮아지지만 현재 승인안은 아닙니다. 현재 세일 상품은 결제 직전 종료 시각과 가격을 다시 확인합니다.

구매는 한꺼번에 하지 않습니다. `0차 무료 Paragon 4종 스테이징 → 1차 Knights (Pack)+검/방패 애니메이션 ₩137,920 → 2차 MONSTERS SERIES BUNDLE ₩162,290 → 3차 월드/VFX ₩162,280 → 4차 오디오 ₩81,110` 순서로 승인합니다. 각 단계가 합성 화면과 실제 전투 검증을 통과하지 못하면 다음 결제를 중단합니다. 세일이 먼저 끝나는 상품은 가격만 다시 비교하고 검증 순서를 건너뛰지 않습니다.

#### 구매하지 않고 최종 제작하는 부분

- UI는 UMG/CommonUI로 체력·전투 스태미나·4기술·어둠 형상·기회공격/처형·보스 HP·목표·설정 화면만 만듭니다. [Input Prompts Pack](https://www.fab.com/listings/7a115c57-73bd-4108-a34c-bb5ec4ac9782)의 입력 아이콘을 사용하고 별도 판타지 UI 킷은 사지 않습니다.
- 환경은 성기사 성채/교회 → 전쟁터/죽은 숲 → 타락한 던전/보스 방의 세 구역으로 제한합니다. 마지막 구역은 환경 팩을 더 사지 않고 검게 탄 재질, 붉은 발광, 연기, 재, 용암 데칼로 변형합니다.
- 조르바·지휘관·보스 장면은 Sequencer와 Control Rig로 짧게 만듭니다. Paragon 원본 대사·캐릭터명·스킬명·UI는 사용하지 않고, 스톡 스토리 음성은 사지 않습니다.
- 로고, 게임 아이콘, 스토어 캡슐, 키아트, 스크린샷, 트레일러, 엔딩 크레딧은 출시 전 별도 제작 산출물로 관리합니다.

#### 제외한 구매 후보

- Hivemind `Dark Fantasy Mega Bundle`은 할인폭은 크지만 현재 필요한 3개 미션 구역보다 범위가 지나치게 넓어 제외합니다.
- Polyart `Stylized Fantasy Castle Environment`는 카툰 비율이 Paragon 주연과 충돌하고 ₩308,380으로 비싸므로 제외합니다.
- `GDH All Animation Bundle`처럼 여러 무기군을 묶은 대형 애니메이션 번들은 현재 검+방패 게임 범위를 넘으므로 사지 않습니다.
- 개별 악마 단품, 애니메이션 없는 모델 팩, 평점이 낮거나 AI 생성인 판타지 UI 킷은 구매 대상에서 제외합니다.

### 3. 구매 승인 조건

- [ ] [Fab Standard License](https://www.fab.com/eula?lang=en)의 최근 12개월 매출·투자금 기준에 맞춰 Personal/Professional 티어를 선택하고 상품 URL·구매일·티어·영수증을 원장에 기록한다.
- [ ] Paragon은 Unreal Engine 프로젝트에서만 사용하고 게임명·광고·스토어 문구에 `PARAGON` 상표를 쓰지 않는다. 원본 캐릭터명·대사·UI·스킬명도 제품에서 제거한다.
- [ ] 먼저 Serath·Greystone·Grux·Sevarog를 빈 UE 5.8 스테이징 프로젝트에 넣어 AnimBP, Physics Asset, 머티리얼, VFX, LOD, 패키징을 확인한다. 통과 전에는 유료 캐릭터를 결제하지 않는다.
- [ ] `Knights (Pack)`은 결제 전에 서로 다른 캐릭터의 부품 교차 호환 여부와 LOD 제공 여부를 판매자 설명/문의로 확인한다.
- [ ] `Sword and Shield Animations V1`은 기사팩 플레이어와 일반병에서 손·발 미끄러짐, 방패 관통, Root Motion 여부와 처형 공격자/피격자 기준 거리를 확인한다.
- [ ] `MONSTERS SERIES BUNDLE`은 UE 5.7 스테이징 프로젝트에서 설치한 뒤 5.8로 변환하고, 사용할 8계열 각각의 기본 공격·피격·경직·사망·이동, Physics Asset, LOD0~2와 패키징을 확인한다.
- [ ] Paragon과 기사/몬스터 팩을 같은 조명에서 나란히 놓고 texel density, roughness, 피부/금속 명도, 그림자, 체형 스케일을 맞춘 합성 화면을 승인한다.
- [ ] 군중전 3~10마리와 보스전에서 재질 수, LOD, Niagara 동시 수, 데칼 수명, 사운드 동시 재생 수를 예산 안에 둔다.
- [ ] 판매자 원본은 `Content/90_ExternalAssets/Marketplace/<Vendor>/<Pack>`에 보존하고, 승인한 에셋만 프로젝트 소유 폴더와 제품 맵으로 옮긴다.

### 4. 프로젝트에 넣은 직후

- [ ] 현재 `Content/SwordAndShieldAnimationV1`인 원본 팩을 골든 히트 승인 뒤 Content Browser로 `Content/90_ExternalAssets/Marketplace` 아래에 옮기고 Redirector를 정리한다.
- [ ] Paragon은 39종 전체가 아니라 Serath·Greystone·Grux·Sevarog의 실제 의존성만 마이그레이션하고 각자의 원본 스켈레톤/AnimBP를 유지한다.
- [ ] UE5 Manny에서 기본 공격 1개, 방어 충격 1개, 피격 1개를 먼저 재생해 무기와 손이 어긋나지 않는지 확인한다.
- [x] 전투용 IK Retargeter를 만들고 이름을 `RTG_Zorba_Combat`로 둔다.
- [ ] 통과한 클립만 플레이어/적의 `Animations/Combat` 폴더로 리타깃한다.
- [ ] 검은 오른손, 방패는 왼손 소켓에 부착하고 장착 상태에서 손목과 팔꿈치 뒤틀림을 확인한다.
- [ ] 주연 캐릭터마다 실루엣·표면·VFX 3축 변형 체크를 통과한 스크린샷을 남긴다.

### 5. 첫 무기 타격

- [x] `UZorbaAttackDefinition`과 `DA_Player_Light01`에 Montage, 방향 정책, 판정 모양·크기, 최대 대상, 체력·스태미나 피해, 전방 보정 값을 둔다.
- [x] 준비·활성·회수 프레임은 Montage Section/Notify가 소유하고 C++/BP에 중복 하드코딩하지 않는다.
- [x] 공격 데이터의 넓은 범위로 대상을 고르고, `Trace_Base`와 `Trace_Tip` Sweep은 타점 보정에만 사용한다.
- [x] 같은 공격에서 같은 대상을 한 번만 맞히고 Player `0`, Enemy `1` 팀 필터를 적용한다.
- [x] 기본 공격 방향, 방향 고정, 적중·빗나감, 범위·각도 안팎, Data Asset Range 반영을 PIE에서 확인한다.
- [x] Montage 진행 중 이동 입력을 차단하고 정상 종료 뒤 즉시 이동이 복구되는지 PIE에서 확인한다.
- [후순위] 강제 중단 복구는 실제 중단 원인을 구현할 때, 판정·사운드·VFX 동기화는 승인된 효과 에셋을 도입할 때 확인한다.

### 6. 다음 구현 순서

- [x] GameplayEffect 초기 수치와 사망/스태미나 고갈 상태 전환 기반
- [x] 적 기본 공격
- [x] 방어와 패리 판정창 구현·자동 실행 증거
- [x] 방어·패리·방어 종료 사용자 PIE 승인
- [x] 강공격과 약공격 후 강공격 파생 구현·자동 실행 증거
- [x] `Exhausted` 기회공격과 제한된 무적 구현·자동 실행 증거
- [x] 고급 공격 4종의 기본 동작 사용자 PIE 승인
- [x] 강공격 중 강공격 재입력 회귀 수정 사용자 PIE 확인
- [x] 졸개 패리 즉결과 약한 적 3명+정예 1명 군중전 구현·자동 실행 증거
- [x] 최소 적 AI 상태/책임 분리와 역할별 Combat Profile 구현·빌드·에셋/맵 실행 검증
- [x] 적 방어, 강공격 가드 브레이크, 패리 유도·회피 전용·장거리 기습 공격과 특수 신호권 구현
- [x] 군중 공격 흐름, 졸개·정예 차이, 즉결처형, 특수 공격 유형 사용자 PIE 승인
- [x] 프레임레이트 의존 이동 수정 후 거리 유지 추격·후퇴·지속 옆걸음과 2.25초 정예 가드 사용자 PIE 승인
- [후속] 적 전용 방어·특수 공격·기습 Montage와 최종 GameplayCue/VFX/SFX
- [x] 첫 신성한 계율: 후방 공격의 방어 무시·체력 피해 증가 패시브 구현·정식 빌드·전후방 자동 비교
- [x] 첫 신성한 계율 전투 게이트 종료 — 기존 전후방 자동 비교와 2026-08-11 사용자 종료 지시
- [x] 첫 금단 기술 슬롯 1: 광폭 패턴·대상 판정·해제/기절·성공 쿨다운 구현·정식 빌드·맵 자동 실행
- [x] 첫 금단 기술 사용자 PIE 승인
- [x] 처형과 체력/전투 스태미나 회복 구현·자동 실행 증거
- [x] M01 데이터·그레이박스 맵과 졸개 3명+정예 1명 조우 배치
- [x] 부트 진입·전멸 성공·플레이어 사망 실패·재시작·부트 복귀와 최소 결과 UI 구현
- [x] 부트→M01 성공, 사망 실패, 재시작, 부트 복귀 자동 여행과 M01 Map Check 검증
- [x] M01 전체 사용자 PIE 승인 — 부트 시작, 성공/실패 결과, 재시작, 부트 복귀, 개발 전투맵 회귀
- [ ] M01 버티컬 슬라이스 폴리시 — 목표 안내, 남은 적 가독성, 공간 동선, 최소 결과 연출과 한 라운드 플레이테스트

## 검증 규칙

- 에디터가 C++ 바이너리를 잡고 있으면 저장하고 종료한 뒤 정식 빌드한다.
- 헤더/UHT 변경은 Live Coding으로 검증하지 않는다.
- 밀접하게 연결된 기능은 내부 컴파일 체크포인트만 두고, 사용자 승인용 PIE는 하나의 플레이 가능한 결과로 묶습니다. 이미 통과한 방향 Arrow 같은 미시 증거는 별도 게이트로 다시 열지 않습니다.
- 통합 PIE 뒤 Output Log와 `git diff`를 확인합니다. 공개 저장소에서는 구매팩 원본과 애니메이션 데이터를 포함한 리타깃 출력물을 제외하고, 에디터 자동 설정이 섞이지 않도록 항상 명시적 경로만 스테이징합니다.
