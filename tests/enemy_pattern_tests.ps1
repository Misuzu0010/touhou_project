$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")

function Read-ProjectFile {
    param([string]$RelativePath)
    return Get-Content -LiteralPath (Join-Path $root $RelativePath) -Raw -Encoding UTF8
}

function Assert-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NoMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -match $Pattern) {
        throw $Message
    }
}

$gameH = Read-ProjectFile "touhou_project/Game.h"
$gameCpp = Read-ProjectFile "touhou_project/Game.cpp"
$enemyH = Read-ProjectFile "touhou_project/Enemy.h"
$strategyH = Read-ProjectFile "touhou_project/BulletStrategy.h"
$gameTypesH = Read-ProjectFile "touhou_project/GameTypes.h"
$stageDirectorH = Read-ProjectFile "touhou_project/StageDirector.h"
$stageEventH = Read-ProjectFile "touhou_project/StageEvent.h"
$projectFile = Read-ProjectFile "touhou_project/touhou_project.vcxproj"
$assetDir = Join-Path $root "touhou_project/assets/image"

Assert-Match $gameTypesH "struct\s+DialogueLine" "GameTypes.h should define DialogueLine for shared enemy dialogue."
Assert-Match $gameTypesH "enum\s+class\s+CharacterID" "GameTypes.h should define CharacterID for shared phase setup."

Assert-Match $strategyH "class\s+BulletStrategy" "BulletStrategy should define a strategy interface."
Assert-Match $strategyH "class\s+RotatingRingStrategy" "Enemy patterns should include rotating ring strategy."
Assert-Match $strategyH "class\s+StopAndGoStrategy" "Enemy patterns should include delayed aim strategy."
Assert-Match $strategyH "class\s+SpiralStrategy" "Enemy patterns should include spiral strategy."
Assert-Match $strategyH "class\s+WideShotStrategy" "Enemy patterns should include wide shot strategy."
Assert-Match $strategyH "class\s+CrossPatternStrategy" "Enemy patterns should include cross pattern strategy."
Assert-Match $strategyH "class\s+BurstStrategy" "Enemy patterns should include burst strategy."
Assert-Match $strategyH "class\s+RainStrategy" "Enemy patterns should include rain strategy."

Assert-Match $enemyH "struct\s+BossPhaseConfig" "Enemy.h should store per-spell phase configuration."
Assert-Match $enemyH "float\s+spellCardHP" "Boss phases should store independent spell HP."
Assert-Match $enemyH "float\s+spellTime" "Boss phases should store spell time limits."
Assert-Match $enemyH "std::unique_ptr<\s*BulletStrategy\s*>" "Boss phases should own a bullet strategy."
Assert-Match $enemyH "void\s+SetupPhases\s*\(" "Enemy should configure phases by selected character."
Assert-Match $enemyH "void\s+SetupReimuPhases\s*\(" "Enemy should configure Reimu boss phases."
Assert-Match $enemyH "void\s+SetupMarisaPhases\s*\(" "Enemy should configure Marisa boss phases."
Assert-Match $enemyH "bool\s+UpdatePhase\s*\(" "Enemy should transition between phases."
Assert-Match $enemyH "void\s+Shoot\s*\(" "Enemy should centralize strategy-based shooting."
Assert-Match $enemyH "GetCurrentSpellHP" "Enemy should expose current spell HP for rendering."
Assert-Match $enemyH "GetTotalRemainingHP" "Enemy should expose total remaining HP for stage events."

Assert-Match $gameCpp "Enemies\[0\]->SetupPhases\s*\(\s*playerID\s*\)" "Battle init should configure enemy phases."
Assert-Match $gameCpp "boss\.UpdatePhase\s*\(" "Game should delegate phase transitions to Enemy."
Assert-NoMatch $gameCpp "CheckEnemyPhase\s*\(\)\s*\{[\s\S]*?if\s*\(\s*isSpellActive\s*\)\s*return" "CheckEnemyPhase should still run while a spell card is active."
Assert-Match $gameCpp "boss\.Shoot\s*\(" "Game should delegate enemy shooting to Enemy."
Assert-Match $gameCpp "boss\.ShouldPlaySfx\s*\(" "Game should query enemy SFX cadence from Enemy."
Assert-Match $gameCpp "GetCurrentSpellHP" "Render should use current spell HP for the boss bar."
Assert-Match $gameCpp "GetSpellTimeRemaining" "Render should show current spell time remaining."
Assert-Match $gameCpp "GetCurrentPhaseIndex" "Render/stage logic should use Enemy phase index."
Assert-Match $gameCpp "assets/image/PlayerBullet\.png" "Game should load moved image assets."
Assert-Match $gameCpp "assets/image/Reimu_b\.png" "Game should load Reimu bomb image."
Assert-Match $gameCpp "assets/image/Marisa_b\.png" "Game should load Marisa bomb image."

if (-not (Test-Path (Join-Path $assetDir "Reimu_b.png"))) {
    throw "Reimu bomb image should exist under touhou_project/assets/image."
}
if (-not (Test-Path (Join-Path $assetDir "Marisa_b.png"))) {
    throw "Marisa bomb image should exist under touhou_project/assets/image."
}
if (Test-Path (Join-Path $root "touhou_project/Reimu.png")) {
    throw "Root-level image assets should be moved to touhou_project/assets/image."
}

Assert-Match $stageDirectorH "class\s+StageDirector" "StageDirector should be defined."
Assert-Match $stageEventH "class\s+TimeEvent" "Stage events should support time triggers."
Assert-Match $stageEventH "class\s+HpThresholdEvent" "Stage events should support HP triggers."
Assert-Match $stageEventH "class\s+PhaseChangeEvent" "Stage events should support phase change triggers."

Assert-Match $gameCpp "SetupStageDirector\s*\(\s*\)" "Battle init should configure StageDirector."
Assert-Match $gameCpp "HandleStageSignals\s*\(\s*ctx\s*\)" "StageDirector signals should be consumed by Game."

Assert-Match $projectFile "StageDirector\.cpp" "vcxproj should compile StageDirector.cpp."
Assert-Match $projectFile "StageEvent\.cpp" "vcxproj should compile StageEvent.cpp."
Assert-Match $projectFile "StageDirector\.h" "vcxproj should include StageDirector.h."
Assert-Match $projectFile "StageEvent\.h" "vcxproj should include StageEvent.h."

Write-Host "Enemy pattern checks passed."
