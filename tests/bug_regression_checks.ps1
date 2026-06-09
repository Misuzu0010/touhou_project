param()

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
$playerH = Read-ProjectFile "touhou_project/Player.h"
$projectFile = Read-ProjectFile "touhou_project/touhou_project.vcxproj"

Assert-Match $gameH "#include\s+<memory>" "Game.h should include <memory> for unique_ptr ownership."
Assert-Match $gameH "std::unique_ptr<Player>\s+player" "Game::player should use std::unique_ptr."
Assert-Match $gameH "std::vector<std::unique_ptr<Enemy>>\s+Enemies" "Enemies should use vector<unique_ptr<Enemy>>."
Assert-Match $gameH "std::vector<std::unique_ptr<Bullet>>\s+playerBullets" "playerBullets should use vector<unique_ptr<Bullet>>."
Assert-Match $gameH "std::vector<std::unique_ptr<Bullet>>\s+enemyBullets" "enemyBullets should use vector<unique_ptr<Bullet>>."
Assert-Match $gameH "std::vector<std::unique_ptr<PowerUp>>\s+powerUps" "powerUps should use vector<unique_ptr<PowerUp>>."
Assert-NoMatch $gameH "std::vector<[^>]+\*>" "Owning vectors in Game.h should not store raw pointers."

Assert-Match $playerH "visualTime\(0\.0f\)" "Player::visualTime should be initialized."
Assert-Match $playerH "currentAngle\(0\.0\)" "Player::currentAngle should be initialized."

Assert-Match $gameCpp "RemoveInactiveObjects\s*\(\s*playerBullets\s*\)" "Update should remove inactive player bullets."
Assert-Match $gameCpp "RemoveInactiveObjects\s*\(\s*enemyBullets\s*\)" "Update should remove inactive enemy bullets."
Assert-Match $gameCpp "RemoveInactiveObjects\s*\(\s*powerUps\s*\)" "Update should remove inactive power ups."
Assert-Match $gameCpp "void\s+Game::ResetRunState\s*\(" "Game should centralize run-state reset logic."
Assert-Match $gameCpp "void\s+Game::ClearBattleObjects\s*\(" "Game should centralize owned object cleanup."
Assert-Match $gameCpp "void\s+Game::PlaySfx\s*\(" "Game should centralize nullable SFX playback."

Assert-Match $gameCpp "void\s+Game::InitBattle[\s\S]*continueTimer\s*=" "InitBattle should reset continueTimer."
Assert-Match $gameCpp "if\s*\(\s*p->IsActive\(\)\s*&&\s*player\s*&&\s*isColliding\s*\)" "PowerUp pickup should use the computed AABB collision."

Assert-NoMatch $gameCpp "se_REIMUBomb\s*&&\s*spellUser\s*==\s*CharacterID::MARISA" "Marisa bomb should not check se_REIMUBomb."
Assert-Match $gameCpp "se_MARISABomb\s*&&\s*spellUser\s*==\s*CharacterID::MARISA" "Marisa bomb should check se_MARISABomb."

Assert-NoMatch $gameCpp "victory\.mov" "Victory SFX should not be loaded from a .mov file with Mix_LoadWAV."
Assert-Match $gameCpp "constexpr\s+int\s+MaxVolume\s*=\s*128" "MaxVolume should be 128."
Assert-Match $gameCpp "bgmVolume\s*<\s*MaxVolume" "BGM volume should be adjustable up to MaxVolume."
Assert-NoMatch $gameCpp "Power:\s+%\.2f\s*/\s*3\.00" "Power display should not claim / 3.00 while max power is 4."

Assert-Match $gameCpp "TTF_CloseFont\s*\(" "Clean should close the loaded font."
Assert-Match $gameCpp "DestroyTexture\s*\(\s*tex_PlayerReimu\s*\)" "Clean should destroy player Reimu texture."
Assert-Match $gameCpp "DestroyTexture\s*\(\s*tex_PlayerMarisa\s*\)" "Clean should destroy player Marisa texture."
Assert-Match $gameCpp "DestroyTexture\s*\(\s*tex_Enemy_Reimu\s*\)" "Clean should destroy enemy Reimu texture."
Assert-Match $gameCpp "DestroyTexture\s*\(\s*tex_Enemy_Marisa\s*\)" "Clean should destroy enemy Marisa texture."
Assert-Match $gameCpp "DestroyTexture\s*\(\s*tex_EnemyBullet\s*\)" "Clean should destroy enemy bullet texture."
Assert-Match $gameCpp "DestroyTexture\s*\(\s*tex_PlayerBullet\s*\)" "Clean should destroy player bullet texture."

Assert-Match $projectFile "<LanguageStandard>stdcpp17</LanguageStandard>" "Project should explicitly use C++17 for modern C++ features."

Write-Host "Bug regression checks passed."
