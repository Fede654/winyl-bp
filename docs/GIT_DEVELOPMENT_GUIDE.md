# Git Development Guide for Winyl

This document provides guidance for developers working with the Winyl repository, including file management, branching strategies, and clean development practices.

## Repository Structure Overview

```
winyl-bp/
├── .gitignore              # Git ignore rules
├── Winyl.sln               # Visual Studio solution
├── Winyl/                  # Main project directory
│   ├── src/                # Source code
│   ├── data/               # Runtime resources
│   └── Winyl.vcxproj       # Project file
├── docs/                   # Documentation (TRACKED)
├── build_and_test.bat      # Build automation (TRACKED)
├── deploy_test.bat         # Deployment script (TRACKED)
└── [ignored directories]   # See gitignore section
```

## File Management Rules

### Files to COMMIT (tracked in repository):

1. **Source Code**: All `.cpp`, `.h`, `.rc` files
2. **Project Files**: `.sln`, `.vcxproj`, `.vcxproj.filters`
3. **Documentation**: Everything in `docs/` directory
4. **Build Scripts**: `build_and_test.bat`, `deploy_test.bat`
5. **Configuration**: `.gitignore`, `CLAUDE.md`, README files
6. **Essential Headers**: BASS header files (`.h` only)

### Files to IGNORE (never commit):

1. **Build Artifacts**:
   - `x64/`, `x86/`, `Debug/`, `Release/` directories
   - `*.obj`, `*.pdb`, `*.dll`, `*.lib` (except essential headers)
   - `build_output.txt`, `build_log_*.txt`

2. **Downloaded Dependencies**:
   - `temp_downloads/` - Temporary build files
   - `bass_downloads/` - BASS library downloads
   - `create_taglib.bat` - Generated build script

3. **Development Tools**:
   - `.claude/` - Claude AI assistant files
   - `.vs/` - Visual Studio cache
   - `*.user` files (except `Winyl.vcxproj.user` which has debugger settings)

4. **Runtime Files**:
   - Profile directories in `Winyl/data/Profile*`
   - Custom skins in `Winyl/data/Skin?*`

## Git Workflow Best Practices

### Before Starting Development

1. **Check repository status**:
   ```bash
   git status
   git pull origin master
   ```

2. **Verify clean working directory**:
   ```bash
   # Should show only intended changes
   git status --porcelain
   ```

### During Development

1. **Stage only relevant files**:
   ```bash
   # Stage specific files
   git add Winyl/src/YourFile.cpp
   git add docs/your-doc.md
   
   # Never use 'git add .' blindly
   ```

2. **Review changes before committing**:
   ```bash
   git diff --cached  # Review staged changes
   git status         # Verify what's being committed
   ```

3. **Use meaningful commit messages**:
   ```bash
   git commit -m "Fix audio playback issue in LibAudio.cpp
   
   - Resolved null pointer exception in BASS initialization
   - Added error handling for unsupported audio formats
   - Updated error logging for better debugging"
   ```

### Build and Test Workflow

1. **Clean build before commits**:
   ```bash
   # Use the automated build script
   ./build_and_test.bat release x64 clean
   ```

2. **Test deployment**:
   ```bash
   # Verify runtime environment
   ./deploy_test.bat release x64
   ```

3. **Commit only after successful testing**:
   ```bash
   git add <your-changes>
   git commit -m "Your commit message"
   ```

## Common Git Scenarios

### Scenario 1: Adding New Source Files

```bash
# Add the new files
git add Winyl/src/NewFeature.cpp
git add Winyl/src/NewFeature.h

# If you updated project file (add to solution)
git add Winyl/Winyl.vcxproj
git add Winyl/Winyl.vcxproj.filters

git commit -m "Add NewFeature implementation"
```

### Scenario 2: Working with Dependencies

```bash
# Dependencies are automatically downloaded by build scripts
# Never commit downloaded files, only update build scripts if needed

# If you need to modify dependency handling:
git add build_and_test.bat  # Only if you modified the script
git commit -m "Update BASS library download process"
```

### Scenario 3: Cleaning Up Untracked Files

```bash
# See what would be removed (dry run)
git clean -nd

# Remove untracked files and directories
git clean -fd

# Remove ignored files too (be careful!)
git clean -fdx
```

### Scenario 4: Handling Build Artifacts

```bash
# These should NEVER be committed:
# - Winyl/x64/Debug/
# - Winyl/x64/Release/
# - bass_downloads/
# - temp_downloads/

# If they appear in git status, add them to .gitignore
echo "your-new-ignore-pattern/" >> .gitignore
git add .gitignore
git commit -m "Update gitignore for new build artifacts"
```

## Development Environment Setup

### 1. Clone and Setup

```bash
# Clone repository
git clone <repository-url> winyl-bp
cd winyl-bp

# First build (downloads dependencies automatically)
./build_and_test.bat debug x64
```

### 2. Daily Development Routine

```bash
# Start of day
git pull origin master
git status  # Check for any uncommitted changes

# After making changes
git add <specific-files>
git commit -m "Descriptive message"

# End of day (optional)
git push origin your-branch-name
```

### 3. Troubleshooting Common Issues

#### Issue: Accidentally committed build artifacts
```bash
# Remove from repository but keep locally
git rm --cached -r Winyl/x64/
git commit -m "Remove build artifacts from repository"

# Make sure they're in .gitignore
echo "Winyl/x64/" >> .gitignore
git add .gitignore
git commit -m "Add build directory to gitignore"
```

#### Issue: Working directory is messy
```bash
# See what's untracked
git status

# Clean untracked files (be careful!)
git clean -fd

# Reset any unwanted changes
git checkout -- <file-to-reset>
```

#### Issue: Large files committed by mistake
```bash
# Remove from history (use with caution)
git filter-branch --tree-filter 'rm -rf bass_downloads' HEAD
git push --force-with-lease origin master
```

## Branching Strategy

### Main Branches
- `master` - Stable, release-ready code
- `develop` - Integration branch for features

### Feature Development
```bash
# Create feature branch
git checkout -b feature/new-audio-format
git push -u origin feature/new-audio-format

# Work on feature...
git add <files>
git commit -m "Implement feature"

# When ready, merge back
git checkout master
git pull origin master
git merge feature/new-audio-format
git push origin master

# Clean up
git branch -d feature/new-audio-format
git push origin --delete feature/new-audio-format
```

## File Size and Repository Health

### Monitoring Repository Size
```bash
# Check repository size
git count-objects -vH

# Find large files
git rev-list --objects --all | sort -k 2 | cut -f 2 -d\  | uniq | while read filename; do echo "$(git rev-list --all --objects -- \"$filename\" | wc -l) $filename"; done | sort -nr
```

### Best Practices for File Size
- Keep binary files to minimum
- Use build scripts to download dependencies
- Document where to get large dependencies
- Consider Git LFS for necessary large files

## Emergency Procedures

### Accidental Commit of Sensitive Data
```bash
# Remove from latest commit
git reset --soft HEAD~1
git reset HEAD <sensitive-file>
git commit -m "Previous commit message"

# Remove from history (nuclear option)
git filter-branch --force --index-filter 'git rm --cached --ignore-unmatch <sensitive-file>' --prune-empty --tag-name-filter cat -- --all
```

### Repository Corruption Recovery
```bash
# Verify repository integrity
git fsck --full

# If issues found, try garbage collection
git gc --aggressive --prune=now

# Last resort: re-clone
git clone <repository-url> winyl-bp-new
```

## Contact and Support

For questions about git workflow or repository issues:
1. Check this documentation first
2. Review git status and recent commits
3. Use `git log --oneline -10` to see recent history
4. Ask team members or maintainers

Remember: **When in doubt, don't commit!** It's easier to add files than to remove them from git history.