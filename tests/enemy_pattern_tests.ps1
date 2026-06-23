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

$gameH = Read-ProjectFile "touhou_project/Game.h"
$gameCpp = Read-ProjectFile "touhou_project/Game.cpp"
$stageDirectorH = Read-ProjectFile "touhou_project/StageDirector.h"
$stageEventH = Read-ProjectFile "touhou_project/StageEvent.h"
$projectFile = Read-ProjectFile "touhou_project/touhou_project.vcxproj"

Assert-Match $gameH "enum\s+class\s+PatternType" "Game.h should define PatternType for enemy patterns."
Assert-Match $gameH "Ring" "PatternType should include Ring."
Assert-Match $gameH "DelayedAim" "PatternType should include DelayedAim."
Assert-Match $gameH "Spiral" "PatternType should include Spiral."

Assert-Match $gameH "float\s+hpThreshold" "EnemyPhase should store hpThreshold."
Assert-Match $gameH "PatternType\s+patternType" "EnemyPhase should store patternType."
Assert-Match $gameH "float\s+shootInterval" "EnemyPhase should store shootInterval."

Assert-Match $gameCpp "p1\.hpThreshold\s*=\s*4000\.0f" "Phase 1 threshold should be 4000 HP."
Assert-Match $gameCpp "p2\.hpThreshold\s*=\s*2000\.0f" "Phase 2 threshold should be 2000 HP."
Assert-Match $gameCpp "PatternType::DelayedAim" "A mid-phase pattern should use DelayedAim."
Assert-Match $gameCpp "PatternType::Spiral" "A low-HP phase should use Spiral."
Assert-Match $gameCpp "PatternType\s+pattern\s*=\s*PatternType::Ring" "The default full-HP phase should use Ring."

Assert-Match $gameCpp "void\s+Game::UpdateBulletPattern\s*\(" "Game.cpp should centralize enemy bullet pattern updates."
Assert-Match $gameCpp "ShootRotatingRing" "UpdateBulletPattern should support ring shots."
Assert-Match $gameCpp "ShootStopAndGo" "UpdateBulletPattern should support delayed aim shots."
Assert-Match $gameCpp "ShootSpiral" "UpdateBulletPattern should support spiral shots."

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
