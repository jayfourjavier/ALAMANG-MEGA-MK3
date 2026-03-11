# Git Workflow Cheat Sheet

## Branch Structure

```
main    → stable public release
dev     → integration branch for development
feature-* → individual features
```

Example:

```
main
 └── dev
      ├── feature-stepper
      ├── feature-lcd
      ├── feature-button
      └── feature-heater
```

---

# 1. Start Working on a Feature

Create a feature branch from `dev`.

```bash
git checkout dev
git pull origin dev
git checkout -b feature-stepper
```

---

# 2. Commit Changes

```bash
git add .
git commit -m "Add stepper helper class"
```

---

# 3. Sync Feature Branch with Dev (IMPORTANT)

Always update your feature branch with the latest `dev`.

```bash
git fetch origin
git merge origin/dev
```

Resolve conflicts if necessary:

```bash
git add .
git commit
```

---

# 4. Push Feature Branch

```bash
git push origin feature-stepper
```

---

# 5. Merge Feature into Dev

Switch to `dev`.

```bash
git checkout dev
git pull origin dev
```

Merge the feature:

```bash
git merge feature-stepper
```

Push the updated `dev` branch:

```bash
git push origin dev
```

---

# 6. Sync Feature Branch After Merge (Optional but Clean)

```bash
git checkout feature-stepper
git merge dev
```

---

# 7. Release to Main

When `dev` is stable:

```bash
git checkout main
git pull origin main
git merge dev
git push origin main
```

---

# Daily Workflow (Quick Version)

Before starting work:

```bash
git checkout feature-stepper
git fetch origin
git merge origin/dev
```

After coding:

```bash
git add .
git commit -m "Describe changes"
git push origin feature-stepper
```

Merge to dev:

```bash
git checkout dev
git merge feature-stepper
git push origin dev
```

---

# Good Practices

* Never commit directly to `main`
* Develop features in `feature-*` branches
* Merge features into `dev`
* Release stable code from `dev` to `main`
* Sync your feature branch with `dev` frequently

---

# Example Workflow

```
dev
 ├── feature-stepper  → add stepper control
 ├── feature-lcd      → add LCD helper
 └── feature-sensor   → HX711 + DHT
```

Each feature branch is merged into `dev` once complete.

---
