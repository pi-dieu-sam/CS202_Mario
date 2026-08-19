#!/usr/bin/env python3
"""
Split large commits on feature/coop-pvp-mode branch into smaller, meaningful commits.
Then rebuild main with the split branch merged in.
"""
import subprocess
import sys
import os

def run(cmd, check=True):
    print(f"  > {cmd}")
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if check and r.returncode != 0:
        print(f"  ERROR: {r.stderr.strip()}")
        sys.exit(1)
    return r.stdout.strip()

def git_add_commit(files, msg):
    """Stage specific files and commit with message."""
    for f in files:
        run(f'git add "{f}"')
    run(f'git commit -m "{msg}"')

# ============================================================
# CONFIGURATION: How to split each large commit
# ============================================================

# d228a9a: "Implement Co-op and PvP modes" (11 files, 312 lines)
SPLIT_D228A9A = [
    {
        "files": ["include/Core/PlayerProgress.hpp"],
        "msg": "feat(core): add GameMode enum to PlayerProgress for Coop and PvP"
    },
    {
        "files": ["include/Core/InputHandler.hpp", "src/Core/InputHandler.cpp"],
        "msg": "feat(input): add Player 2 keybindings with arrow keys and numpad controls"
    },
    {
        "files": ["include/Core/Camera.hpp", "src/Core/Camera.cpp"],
        "msg": "feat(camera): add dual-player camera tracking with midpoint interpolation"
    },
    {
        "files": ["include/Level/Level.hpp", "src/Level/Level.cpp"],
        "msg": "feat(level): add Player 2 entity management and multiplayer collision logic"
    },
    {
        "files": ["include/States/MenuState.hpp", "src/States/MenuState.cpp"],
        "msg": "feat(menu): add 2-Player Co-op and 2-Player PvP menu options"
    },
    {
        "files": ["include/States/PlayingState.hpp", "src/States/PlayingState.cpp"],
        "msg": "feat(playing): initialize and render Player 2 in multiplayer modes"
    },
]

# 78bc2f7: "Split Coop/PvP modes" (4 files, 74 lines)
SPLIT_78BC2F7 = [
    {
        "files": ["include/Core/PlayerProgress.hpp"],
        "msg": "feat(core): differentiate Coop and PvP collision rules in PlayerProgress"
    },
    {
        "files": ["src/Level/Level.cpp"],
        "msg": "feat(level): disable friendly fire in Co-op and enable stomp damage in PvP"
    },
    {
        "files": ["include/States/MenuState.hpp", "src/States/MenuState.cpp"],
        "msg": "feat(menu): update menu transitions for separated Co-op and PvP flows"
    },
]

# 16d34f1: "Add PvP arena map and dedicated PvP mode with winner screen" (7 files, 97 lines)
SPLIT_16D34F1 = [
    {
        "files": ["assets/levels/pvp_arena.txt"],
        "msg": "feat(level): create PvP arena map with platforms and pits"
    },
    {
        "files": ["include/Level/LevelLoader.hpp", "src/Level/LevelLoader.cpp"],
        "msg": "feat(loader): add PvP arena level loading support"
    },
    {
        "files": ["include/States/GameOverState.hpp", "src/States/GameOverState.cpp"],
        "msg": "feat(gameover): add PvP winner announcement screen with player name display"
    },
    {
        "files": ["src/Level/Level.cpp", "src/States/PlayingState.cpp"],
        "msg": "feat(playing): route PvP mode to dedicated arena map and handle PvP game end"
    },
]

# 58f86dd: "Fix PvP death logic to isolate players and revamp PvP winner screen" (8 files, 99 lines)
SPLIT_58F86DD = [
    {
        "files": ["include/Entities/Player.hpp", "src/Entities/Player.cpp"],
        "msg": "feat(player): add per-player alive state tracking for independent PvP deaths"
    },
    {
        "files": ["include/Observers/GameEvents.hpp"],
        "msg": "feat(events): add playerId field to GameEvent for multiplayer event routing"
    },
    {
        "files": ["src/Level/Level.cpp"],
        "msg": "fix(level): isolate player death handling so one player dying does not kill the other"
    },
    {
        "files": ["include/States/GameOverState.hpp", "src/States/GameOverState.cpp"],
        "msg": "feat(gameover): add pulsing animation effect to PvP winner text"
    },
    {
        "files": ["assets/levels/pvp_arena.txt", "src/States/PlayingState.cpp"],
        "msg": "fix(pvp): adjust arena spawn points and freeze surviving player during death animation"
    },
]

# Map commit hashes to their split config
SPLITS = {
    "d228a9a": SPLIT_D228A9A,
    "78bc2f7": SPLIT_78BC2F7,
    "16d34f1": SPLIT_16D34F1,
    "58f86dd": SPLIT_58F86DD,
}

# Original commit order on the branch (oldest first)
BRANCH_COMMITS = [
    "d228a9a",  # Implement Co-op and PvP modes
    "78bc2f7",  # Split Coop/PvP modes
    "16d34f1",  # Add PvP arena map
    "58f86dd",  # Fix PvP death logic
    "4705f26",  # Fix P2 spawn position
    "cb6ef49",  # Fix PvP player freeze
    "7a65407",  # Remove TOP-score UI
    "4ea1a73",  # Fix mode transitions keyboard
    "5d755d3",  # Fix mode transitions mouse
]

# ============================================================
# MAIN LOGIC
# ============================================================

# The base of the feature branch (parent of first commit on the branch)
BRANCH_BASE = run("git rev-parse d228a9a~1")
# The merge commit for PR #37
MERGE_COMMIT = "9687a9d"
# Parent of merge on main side
MERGE_MAIN_PARENT = run(f"git rev-parse {MERGE_COMMIT}~1")

print(f"Branch base: {BRANCH_BASE}")
print(f"Merge main parent: {MERGE_MAIN_PARENT}")

# Step 1: Create the new split branch
NEW_BRANCH = "feature/coop-pvp-mode-split"
run(f"git branch -D {NEW_BRANCH}", check=False)
run(f"git checkout -b {NEW_BRANCH} {BRANCH_BASE}")

# Step 2: Replay each commit, splitting the large ones
for commit_hash in BRANCH_COMMITS:
    short = commit_hash[:7]
    
    if short in SPLITS:
        print(f"\n=== SPLITTING {short} ===")
        split_config = SPLITS[short]
        
        # Apply the full commit's changes without committing
        run(f"git cherry-pick {commit_hash} --no-commit")
        
        # Unstage everything
        run("git reset HEAD")
        
        # Commit in pieces
        for piece in split_config:
            # Check which files actually exist/changed
            existing_files = []
            for f in piece["files"]:
                status = run(f'git status --porcelain "{f}"', check=False)
                if status:
                    existing_files.append(f)
            
            if existing_files:
                git_add_commit(existing_files, piece["msg"])
            else:
                print(f"  SKIP (no changes): {piece['msg']}")
        
        # Commit any remaining unstaged files (safety net)
        remaining = run("git status --porcelain", check=False)
        if remaining:
            print(f"  Committing remaining files from {short}")
            run("git add -A")
            orig_msg = run(f'git log --format="%s" -1 {commit_hash}')
            run(f'git commit -m "chore: remaining changes from {orig_msg}"')
    else:
        print(f"\n=== CHERRY-PICK {short} (keep as-is) ===")
        run(f"git cherry-pick {commit_hash}")

print("\n=== Branch split complete! ===")
new_tip = run("git rev-parse HEAD")
print(f"New branch tip: {new_tip}")

# Step 3: Rebuild main with the new split branch
# Find what comes after the original merge on main
print("\n=== Rebuilding main ===")

# Checkout main and reset to merge's main-side parent
run("git checkout main")

# Save current main tip
old_main_tip = run("git rev-parse main")

# Reset main to just before the original PR #37 merge
run(f"git reset --hard {MERGE_MAIN_PARENT}")

# Create a new merge commit with the split branch
run(f'git merge {NEW_BRANCH} --no-ff -m "Merge feature/coop-pvp-mode: Co-op and PvP multiplayer modes"')

# Now cherry-pick all commits that were AFTER the original merge on old main
# Get list of commits after merge (first-parent only)
after_merge = run(f"git log --oneline --first-parent --reverse {MERGE_COMMIT}..{old_main_tip}")
if after_merge:
    for line in after_merge.split("\n"):
        if not line.strip():
            continue
        ch = line.split()[0]
        orig_msg = run(f'git log --format="%s" -1 {ch}')
        
        # Check if it's a merge commit (has 2+ parents)
        parent_count = run(f'git rev-list --parents -1 {ch}', check=False).split()
        is_merge = len(parent_count) > 2  # hash + 2 parents = merge
        if is_merge:
            # It's a merge commit - get the second parent
            second_parent = parent_count[2]
            print(f"  Merging: {orig_msg}")
            result = subprocess.run(
                f'git merge {second_parent} --no-ff -m "{orig_msg}"',
                shell=True, capture_output=True, text=True
            )
            if result.returncode != 0:
                # If merge conflicts, try to use the original tree
                print(f"  Merge conflict, using original tree for: {orig_msg}")
                run("git merge --abort", check=False)
                # Use the original commit's tree
                tree = run(f"git log --format=%T -1 {ch}")
                run(f'git merge {second_parent} --no-ff -s ours -m "{orig_msg}"')
                # Now replace tree
                run(f'git reset --soft HEAD~1')
                parents_str = run("git rev-parse HEAD") + " " + second_parent
                new_commit = run(f'git commit-tree {tree} -p {run("git rev-parse HEAD")} -p {second_parent} -m "{orig_msg}"')
                run(f"git reset --hard {new_commit}")
        else:
            # Regular commit
            print(f"  Cherry-picking: {orig_msg}")
            result = subprocess.run(
                f"git cherry-pick {ch}",
                shell=True, capture_output=True, text=True
            )
            if result.returncode != 0:
                print(f"  Cherry-pick conflict, using original changes")
                run("git cherry-pick --abort", check=False)
                # Apply with theirs strategy
                run(f"git cherry-pick {ch} --strategy-option=theirs", check=False)

# Final verification
print("\n=== VERIFICATION ===")
# Compare final tree with backup
diff = run(f"git diff backup-before-split main --stat", check=False)
if diff:
    print(f"Differences from backup:\n{diff}")
else:
    print("PERFECT: No code differences from backup!")

new_count = run("git log --oneline main | wc -l", check=False)
if not new_count:
    new_count = len(run("git log --oneline main").split("\n"))
print(f"Total commits on main: {new_count}")

phat_count = len(run('git log --oneline --author="Phat" --no-merges main').split("\n"))
print(f"Phat's commits: {phat_count}")
