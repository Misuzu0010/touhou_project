# SpellCard System Test Suite
# Tests the new SpellCard architecture for correctness and integration

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")

function Read-ProjectFile {
    param([string]$RelativePath)
    return Get-Content -LiteralPath (Join-Path $root $RelativePath) -Raw -Encoding UTF8
}

function Assert-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) { throw "FAIL: $Message" }
}

function Assert-NoMatch {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -match $Pattern) { throw "FAIL: $Message" }
}

$spellCardH   = Read-ProjectFile "touhou_project/SpellCard.h"
$spellCardCpp = Read-ProjectFile "touhou_project/SpellCard.cpp"
$reimuH       = Read-ProjectFile "touhou_project/ReimuSpellCard.h"
$reimuCpp     = Read-ProjectFile "touhou_project/ReimuSpellCard.cpp"
$marisaH      = Read-ProjectFile "touhou_project/MarisaSpellCard.h"
$marisaCpp    = Read-ProjectFile "touhou_project/MarisaSpellCard.cpp"
$gameH        = Read-ProjectFile "touhou_project/Game.h"
$gameCpp      = Read-ProjectFile "touhou_project/Game.cpp"
$playerH      = Read-ProjectFile "touhou_project/Player.h"
$projectFile  = Read-ProjectFile "touhou_project/touhou_project.vcxproj"

$testsPassed = 0
$testsTotal = 0

function Run-Test {
    param([scriptblock]$Test)
    $script:testsTotal++
    try {
        & $Test
        $script:testsPassed++
    } catch {
        Write-Host $_.Exception.Message -ForegroundColor Red
    }
}

# ===== 1. SpellCard.h Structure =====
Write-Host "===== 1. SpellCard.h Structure =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $spellCardH "class\s+Player" "SpellCard.h should forward-declare Player" }
Run-Test { Assert-Match   $spellCardH "class\s+Enemy"  "SpellCard.h should forward-declare Enemy" }
Run-Test { Assert-Match   $spellCardH "class\s+Bullet" "SpellCard.h should forward-declare Bullet" }
Run-Test { Assert-Match   $spellCardH "struct\s+SpellContext" "SpellCard.h should define SpellContext" }
Run-Test { Assert-Match   $spellCardH "enemyBullets\s*=\s*nullptr" "SpellContext should have enemyBullets pointer" }
Run-Test { Assert-Match   $spellCardH "boss\s*=\s*nullptr" "SpellContext should have boss pointer" }
Run-Test { Assert-Match   $spellCardH "player\s*=\s*nullptr" "SpellContext should have player pointer" }
Run-Test { Assert-Match   $spellCardH "renderer\s*=\s*nullptr" "SpellContext should have renderer pointer" }
Run-Test { Assert-Match   $spellCardH "class\s+SpellCard" "SpellCard.h should define SpellCard class" }
Run-Test { Assert-Match   $spellCardH "virtual\s+void\s+Activate\s*\(\s*\)\s*=\s*0" "SpellCard should have pure virtual Activate" }
Run-Test { Assert-Match   $spellCardH "virtual\s+void\s+Update\s*\(\s*float\s+deltaTime\s*\)" "SpellCard should have virtual Update" }
Run-Test { Assert-Match   $spellCardH "virtual\s+void\s+Render\s*\(\s*SDL_Renderer" "SpellCard should have pure virtual Render" }
Run-Test { Assert-Match   $spellCardH "virtual\s+bool\s+IsFinished" "SpellCard should have virtual IsFinished" }
Run-Test { Assert-Match   $spellCardH "virtual\s+.*\s+GetName\s*\(\s*\)\s*=\s*0" "SpellCard should have pure virtual GetName" }
Run-Test { Assert-Match   $spellCardH "m_duration\s*=\s*0" "SpellCard should init m_duration to 0" }
Run-Test { Assert-Match   $spellCardH "m_isFinished\s*=\s*false" "SpellCard should init m_isFinished to false" }
Run-Test { Assert-Match   $spellCardH "m_maxDuration" "SpellCard should have m_maxDuration member" }

# ===== 2. SpellCard.cpp Implementation =====
Write-Host "===== 2. SpellCard.cpp Implementation =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $spellCardCpp "#include\s+['""]Enemy\.h['""]" "SpellCard.cpp should include Enemy.h for hit()" }
Run-Test { Assert-Match   $spellCardCpp "void\s+SpellCard::Update" "SpellCard.cpp should implement Update" }
Run-Test { Assert-Match   $spellCardCpp "m_ctx\.boss.*hit\s*\(" "SpellCard::Update should call boss->hit()" }
Run-Test { Assert-Match   $spellCardCpp "m_isFinished\s*=\s*true" "SpellCard::Update should set m_isFinished when duration exceeded" }

# ===== 3. ReimuSpellCard =====
Write-Host "===== 3. ReimuSpellCard =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $reimuH "class\s+ReimuSpellCard\s*:\s*public\s+SpellCard" "ReimuSpellCard should inherit SpellCard" }
Run-Test { Assert-Match   $reimuH "void\s+Activate\s*\(\s*\)\s*override" "ReimuSpellCard should override Activate" }
Run-Test { Assert-Match   $reimuH "void\s+Update\s*\(\s*float\s+deltaTime\s*\)\s*override" "ReimuSpellCard should override Update" }
Run-Test { Assert-Match   $reimuH "void\s+Render\s*\(\s*SDL_Renderer" "ReimuSpellCard should override Render" }
Run-Test { Assert-Match   $reimuH "m_angles" "ReimuSpellCard should have m_angles member" }
Run-Test { Assert-Match   $reimuH "m_rotateSpeed" "ReimuSpellCard should have m_rotateSpeed member" }
Run-Test { Assert-Match   $reimuH "m_orbitRadius" "ReimuSpellCard should have m_orbitRadius member" }

Run-Test { Assert-Match   $reimuCpp "Deactivate\s*\(\s*\)" "ReimuSpellCard::Activate should deactivate enemy bullets" }
Run-Test { Assert-Match   $reimuCpp "SetInvincible\s*\(\s*3\.5f\s*\)" "ReimuSpellCard::Activate should set 3.5s invincibility" }
Run-Test { Assert-Match   $reimuCpp "SpellCard::Update" "ReimuSpellCard::Update should call base class Update" }
Run-Test { Assert-Match   $reimuCpp "m_rotateSpeed\s*\*\s*deltaTime" "ReimuSpellCard::Update should rotate angles by speed*dt" }
Run-Test { Assert-Match   $reimuCpp "std::cos" "ReimuSpellCard::Render should use cos for orbit position" }
Run-Test { Assert-Match   $reimuCpp "std::sin" "ReimuSpellCard::Render should use sin for orbit position" }
Run-Test { Assert-Match   $reimuCpp "255,\s*100,\s*200" "ReimuSpellCard::Render should use pink color (255,100,200)" }

# ===== 4. MarisaSpellCard =====
Write-Host "===== 4. MarisaSpellCard =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $marisaH "class\s+MarisaSpellCard\s*:\s*public\s+SpellCard" "MarisaSpellCard should inherit SpellCard" }
Run-Test { Assert-Match   $marisaH "m_laserWidth" "MarisaSpellCard should have m_laserWidth member" }
Run-Test { Assert-Match   $marisaH "m_flickerTimer" "MarisaSpellCard should have m_flickerTimer member" }

Run-Test { Assert-Match   $marisaCpp "abs\s*\(.*GetPosition.*x.*-\s*playerX" "MarisaSpellCard::Activate should clear bullets within player X range" }
Run-Test { Assert-Match   $marisaCpp "SetInvincible\s*\(\s*3\.5f\s*\)" "MarisaSpellCard::Activate should set 3.5s invincibility" }
Run-Test { Assert-Match   $marisaCpp "m_flickerTimer\s*\+=\s*deltaTime" "MarisaSpellCard::Update should increment flickerTimer" }
Run-Test { Assert-Match   $marisaCpp "SDL_BLENDMODE_ADD" "MarisaSpellCard::Render should use additive blending for laser" }
Run-Test { Assert-Match   $marisaCpp "sin\s*\(\s*m_flickerTimer" "MarisaSpellCard::Render should use sin for flicker effect" }
Run-Test { Assert-Match   $marisaCpp "255,\s*255,\s*200" "MarisaSpellCard::Render should use white-yellow for core beam" }

# ===== 5. Game.h Integration =====
Write-Host "===== 5. Game.h Integration =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $gameH "SpellCard\.h" "Game.h should include SpellCard.h" }
Run-Test { Assert-Match   $gameH "std::unique_ptr<SpellCard>\s+activeSpell" "Game should have unique_ptr<SpellCard> activeSpell" }
Run-Test { Assert-NoMatch $gameH "isSpellActive" "Game.h should not have old isSpellActive member" }
Run-Test { Assert-NoMatch $gameH "spellTimer" "Game.h should not have old spellTimer member" }
Run-Test { Assert-NoMatch $gameH "spellUser" "Game.h should not have old spellUser member" }

# ===== 6. Game.cpp Integration =====
Write-Host "===== 6. Game.cpp Integration =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $gameCpp "ReimuSpellCard\.h" "Game.cpp should include ReimuSpellCard.h" }
Run-Test { Assert-Match   $gameCpp "MarisaSpellCard\.h" "Game.cpp should include MarisaSpellCard.h" }
Run-Test { Assert-Match   $gameCpp "activeSpell\.reset\s*\(\s*\)" "Game.cpp should reset activeSpell in InitBattle" }
Run-Test { Assert-Match   $gameCpp "SpellContext\s+ctx" "Game.cpp should create SpellContext for bomb" }
Run-Test { Assert-Match   $gameCpp "ctx\.enemyBullets\s*=\s*&enemyBullets" "SpellContext should bind enemyBullets" }
Run-Test { Assert-Match   $gameCpp "ctx\.boss\s*=" "SpellContext should bind boss" }
Run-Test { Assert-Match   $gameCpp "ctx\.player\s*=" "SpellContext should bind player" }
Run-Test { Assert-Match   $gameCpp "ctx\.renderer\s*=\s*cur_Renderer" "SpellContext should bind renderer" }
Run-Test { Assert-Match   $gameCpp "std::make_unique<ReimuSpellCard>" "Game should create ReimuSpellCard for Reimu" }
Run-Test { Assert-Match   $gameCpp "std::make_unique<MarisaSpellCard>" "Game should create MarisaSpellCard for Marisa" }
Run-Test { Assert-Match   $gameCpp "activeSpell->Activate\s*\(\s*\)" "Game should call Activate on new spell" }
Run-Test { Assert-Match   $gameCpp "activeSpell->Update\s*\(" "Game should Update activeSpell in PLAYING state" }
Run-Test { Assert-Match   $gameCpp "activeSpell->IsFinished\s*\(\s*\)" "Game should check IsFinished to end spell" }
Run-Test { Assert-Match   $gameCpp "activeSpell->Render\s*\(\s*cur_Renderer\s*\)" "Game should Render activeSpell" }
Run-Test { Assert-NoMatch $gameCpp "isSpellActive\s*=\s*true" "Game.cpp should not set old isSpellActive" }
Run-Test { Assert-NoMatch $gameCpp "spellTimer\s*=" "Game.cpp should not use old spellTimer" }

# ===== 7. Player.h Friend Access =====
Write-Host "===== 7. Player.h Friend Access =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $playerH "friend\s+class\s+SpellCard" "Player should friend SpellCard" }
Run-Test { Assert-Match   $playerH "friend\s+class\s+ReimuSpellCard" "Player should friend ReimuSpellCard" }
Run-Test { Assert-Match   $playerH "friend\s+class\s+MarisaSpellCard" "Player should friend MarisaSpellCard" }

# ===== 8. Project File =====
Write-Host "===== 8. Project File =====" -ForegroundColor Yellow

Run-Test { Assert-Match   $projectFile "SpellCard\.cpp" "vcxproj should include SpellCard.cpp" }
Run-Test { Assert-Match   $projectFile "ReimuSpellCard\.cpp" "vcxproj should include ReimuSpellCard.cpp" }
Run-Test { Assert-Match   $projectFile "MarisaSpellCard\.cpp" "vcxproj should include MarisaSpellCard.cpp" }
Run-Test { Assert-Match   $projectFile "SpellCard\.h" "vcxproj should include SpellCard.h" }
Run-Test { Assert-Match   $projectFile "ReimuSpellCard\.h" "vcxproj should include ReimuSpellCard.h" }
Run-Test { Assert-Match   $projectFile "MarisaSpellCard\.h" "vcxproj should include MarisaSpellCard.h" }

# ===== 9. Build Verification =====
Write-Host "===== 9. Build Verification =====" -ForegroundColor Yellow

$exePath = Join-Path $root "x64\Release\touhou_project.exe"
Run-Test {
    if (-not (Test-Path $exePath)) { throw "FAIL: Release exe not found at $exePath" }
}

# ===== Summary =====
Write-Host ""
Write-Host "===== Results =====" -ForegroundColor Cyan
Write-Host "Passed: $testsPassed / $testsTotal" -ForegroundColor $(if ($testsPassed -eq $testsTotal) { "Green" } else { "Red" })
if ($testsPassed -eq $testsTotal) {
    Write-Host "ALL TESTS PASSED!" -ForegroundColor Green
} else {
    Write-Host "SOME TESTS FAILED!" -ForegroundColor Red
}
