# 현재 제작 체크리스트

이 파일만 현재 작업 순서와 다음 재개 지점을 관리합니다. 날짜별 완료 이력은 누적하지 않고, 아래 `현재 작업 인계`를 작업 종료 때마다 최신 상태로 덮어씁니다. 다음 작업은 반드시 이 인계를 먼저 읽고 시작하며, 이미 승인된 증거를 별도 단계로 다시 열지 않습니다.

## 현재 작업 인계 — 2026-07-22 종료

현재 활성 게이트는 `5. 첫 무기 타격`의 **기본 공격 골든 히트**입니다. 에셋 추가 구매, 보스, 악마, 방어·패리로 퍼지지 않고 먼저 `입력 → 공격 방향 고정 → Montage → 판정 → 1회 피해 → 피격 반응`을 한 흐름으로 완성합니다.

### 오늘 여기까지

완료·실증:

- `Sword and Shield Animations V1`의 IP 135개와 RM 152개, 총 287개 애니메이션을 UE 5.8 프로젝트에 반입했습니다.
- `IK_SwordShield_Manny`, `RTG_Zorba_Combat`을 만들고 `Idle1_IP`, `Attack1_IP` 두 클립을 Garret으로 리타깃했습니다.
- `Combat.FullBody` Slot과 `AM_Zorba_SS_Light01`을 만들었고 입력으로 Montage가 정상 재생됩니다.
- Montage에는 `AttackActive` Notify Window와 `HitCommit` Notify가 있습니다.
- `UZorbaAttackDefinition`, `DA_Player_Light01`, `UZorbaMeleeCombatComponent`를 만들고 정식 C++ 빌드가 통과했습니다.
- 이동 입력이 있으면 현재 `MoveAction`이 뜻하는 월드 이동 방향, 입력이 없으면 캐릭터 전방을 공격 방향으로 선택합니다. 청록색 Arrow가 W/A/S/D와 대각선 입력을 따라 바뀌는 PIE 증거까지 통과했습니다.
- 정수리 카메라에서 수평 Forward가 0에 가까워질 때 Right 벡터로 지면 Forward를 복원하는 fallback은 코드에 포함했습니다. 이 극단각은 내일 통합 전투 검증 안에서만 회귀 확인합니다.

임시 승인:

- `Attack1`의 길이와 `AttackActive`/`HitCommit` 위치는 현재 전투를 만들 수 있는 수준으로만 승인했습니다. 실제 타격을 붙인 뒤 접촉 프레임을 한 번 조정합니다.
- 검·방패 부착과 `Trace_Base`/`Trace_Tip` 위치는 작동하지만 정렬이 어색합니다. 골든 히트를 막지는 않으며 캐릭터 최종 교체 전 별도 보정합니다.
- 판매자 원본은 현재 `Content/SwordAndShieldAnimationV1`에 있습니다. 참조가 연결된 상태에서 파일 시스템으로 옮기지 말고, 골든 히트 뒤 Content Browser 이동과 Redirector 정리로 `90_ExternalAssets` 규칙을 맞춥니다. 공개 저장소에는 원본과 애니메이션 데이터를 포함한 리타깃 출력물을 올리지 않습니다.

아직 미구현:

- Data Asset Montage의 단일 실행 권한, 실제 캐릭터 회전과 공격 중 방향 잠금, 연타 재시작 차단, 정상 종료·중단 시 정리
- Notify의 C++ 수신, `ForwardArc` 대상 선정, 활성 구간의 무기 Sweep, 동일 대상 1회 및 팀 필터
- 실제 Health/Stamina 피해, 피격 반응, 사운드·VFX
- `Content/30_Enemies`의 재사용 가능한 적 전투 기반. 현재는 `.gitkeep`뿐이므로 실제 피해 증거를 낼 대상이 없습니다.

### 고정 전투 판정 계약

1. **타격 대상은 공격별 넓은 범위가 결정합니다.** `DA_Player_Light01.HitPhases[0]`의 `Shape`, `Range`, `HalfAngleDegrees`, `HalfHeight`, `MaxTargets`를 저장된 공격 방향에 적용합니다.
2. **무기 Trace는 대상 선정 권한이 없습니다.** `Trace_Base`와 `Trace_Tip` Sweep은 타점·법선·피격 부위·VFX 위치를 보정합니다. 넓은 범위에 든 대상은 얇은 검 Trace가 빗나가도 피해 대상에서 탈락시키지 않습니다.
3. Montage·범위·피해·방향 정책은 `UZorbaAttackDefinition`이 소유합니다. 준비·활성·회수의 실제 프레임 타이밍은 Montage Section/Notify가 소유하며 C++이나 BP에 초 단위로 복제하지 않습니다.
4. `bAttackInProgress`는 Montage 전체의 재입력 차단·방향 잠금을, `bHitWindowOpen`은 `AttackActive` 동안의 접점 수집만 담당합니다.
5. BP와 C++에서 Montage를 이중 재생하지 않습니다. 내일부터 `DA_Player_Light01.Montage`가 유일한 소스이고, 현재 BP의 하드코딩 `Play Anim Montage` 경로는 제거합니다.

### 내일 첫 작업 — 한 묶음으로 구현

헤더/UHT 변경이 포함되므로 먼저 에디터를 저장·종료합니다. 시작 파일은 `UZorbaMeleeCombatComponent::BeginAttack()`입니다. 아래 1~8을 서로 다른 사용자 승인 단계로 잘게 쪼개지 않고, 내부 컴파일 체크포인트만 두면서 **기능적 골든 히트 한 묶음**으로 진행합니다.

1. `DA_Player_Light01.Montage`를 로드·재생하는 책임을 C++ 공격 경로로 옮기고 BP의 하드코딩 Montage 재생을 제거합니다.
2. 공격 시작 시 현재 방향을 한 번 저장하고 그 방향으로 캐릭터를 회전시킨 뒤, Montage가 끝날 때까지 방향과 재입력을 잠급니다.
3. 정상 종료, Blend Out, 중단의 모든 경로에서 잠금·활성 공격·Trace 상태가 반드시 해제되게 합니다.
4. `AttackActive` 시작/종료와 `HitCommit`을 C++ 컴포넌트가 받아 `bHitWindowOpen`과 판정 커밋을 제어하게 합니다.
5. `ForwardArc`로 넓은 후보를 모으고 자기 자신, 아군, 사망 대상, 각도 밖 대상, 이미 맞은 대상을 제외합니다.
6. 활성 구간에만 이전/현재 `Trace_Base`·`Trace_Tip` 사이를 Sweep하여 접촉 정보를 모읍니다. 접촉이 없으면 넓은 판정 대상의 충돌체에서 대체 타점을 구합니다.
7. 버릴 테스트 더미 대신 이후 AI가 그대로 상속할 최소 `Enemy` 전투 기반을 만듭니다. Team `1`, ASC/Attribute, Health/Stamina, 피격 반응을 소유하게 하고 첫 검증에서는 맵에 정지 상태로 배치합니다.
8. `HitCommit`에서 대상별 한 번만 GameplayEffect와 피격 반응을 적용하고, 타점에 디버그 표시를 남깁니다. 승인된 SFX/VFX가 없으면 일회용 에셋을 만들지 않고 해당 표현만 다음 증거로 남깁니다.

방향 Arrow만 다시 확인하는 별도 테스트는 하지 않습니다. 다만 새로 구현되는 **회전·잠금·해제**는 다음 한 번의 통합 PIE에서 공격 결과와 함께 확인합니다.

### 내일 묶음의 단일 통합 PIE 승인 조건

- W/A/S/D·대각선 입력 중 공격은 입력 방향으로 회전해 타격하고, 무입력 공격은 현재 캐릭터 전방을 유지합니다.
- 공격 중 반대 방향을 입력해도 진행 중인 공격 방향은 바뀌지 않고, 연타로 Montage가 처음부터 재시작되지 않습니다.
- 정상 종료와 강제 중단 뒤 이동·회전·다음 공격이 모두 복구됩니다.
- 넓은 `ForwardArc` 안의 적은 검 Trace가 직접 스치지 않아도 맞고, 범위·각도 밖의 적은 맞지 않습니다.
- 한 공격에서 같은 적의 Health/Stamina가 정확히 한 번만 감소하고 피격 반응도 한 번만 재생됩니다.
- `DA_Player_Light01`의 Range를 바꾸면 코드 수정 없이 실제 범위가 바뀝니다.
- 정수리 카메라 fallback을 포함해 크래시·경고·잠금 잔류 없이 Output Log가 깨끗합니다.
- 이 증거가 통과한 뒤에만 `적 기본 공격 → 방어·패리 → 강공격·Exhausted → 기회공격` 순서로 넘어갑니다.

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
- [ ] 준비·활성·회수 프레임은 Montage Section/Notify가 소유하고 C++/BP에 중복 하드코딩하지 않는다.
- [ ] 공격 데이터의 넓은 범위로 대상을 고르고, `Trace_Base`와 `Trace_Tip` Sweep은 타점 보정에만 사용한다.
- [ ] 같은 공격에서 같은 대상을 한 번만 맞히고 Player `0`, Enemy `1` 팀 필터를 적용한다.
- [ ] 기본 공격 적중, 빗나감, 방어 충격, 패리, 피격 경직을 PIE에서 확인한다.
- [ ] 검이 닿았다고 보이는 순간에 판정·사운드·피격 반응이 함께 나는 골든 히트를 승인한다.

### 6. 다음 구현 순서

- [ ] GameplayEffect 초기 수치와 사망/스태미나 고갈/`Exhausted` 상태 전환
- [ ] 적 기본 공격과 패리 판정창
- [ ] 강공격과 약공격 후 강공격 파생
- [ ] `Exhausted` 기회공격과 제한된 무적
- [ ] 약한 적 3~5마리 군중전
- [ ] 신성한 계율 1개와 금단 기술 1개
- [ ] 처형과 체력/전투 스태미나 회복
- [ ] 최소 전투 UI와 미션 성공/실패 흐름

## 검증 규칙

- 에디터가 C++ 바이너리를 잡고 있으면 저장하고 종료한 뒤 정식 빌드한다.
- 헤더/UHT 변경은 Live Coding으로 검증하지 않는다.
- 밀접하게 연결된 기능은 내부 컴파일 체크포인트만 두고, 사용자 승인용 PIE는 하나의 플레이 가능한 결과로 묶습니다. 이미 통과한 방향 Arrow 같은 미시 증거는 별도 게이트로 다시 열지 않습니다.
- 통합 PIE 뒤 Output Log와 `git diff`를 확인합니다. 공개 저장소에서는 구매팩 원본과 애니메이션 데이터를 포함한 리타깃 출력물을 제외하고, 에디터 자동 설정이 섞이지 않도록 항상 명시적 경로만 스테이징합니다.
