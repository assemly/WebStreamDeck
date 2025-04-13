# Git提交规范指南

## 目录
1. [引言](#引言)
2. [提交信息格式](#提交信息格式)
3. [提交类型](#提交类型)
4. [提交示例](#提交示例)
5. [分支管理策略](#分支管理策略)
6. [GitHub特定的提交规范](#github特定的提交规范)
7. [提交最佳实践](#提交最佳实践)
8. [提交前检查](#提交前检查)
9. [使用工具辅助](#使用工具辅助)

## 引言

规范的Git提交对于项目的版本控制和团队协作至关重要，它能提高代码审查效率，并使项目历史更加清晰。本指南旨在提供详细的Git提交规范，帮助团队保持一致的版本控制实践，便于项目管理和协作。

## 提交信息格式

采用[约定式提交(Conventional Commits)](https://www.conventionalcommits.org/)规范，基本格式如下：

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

- **type**: 提交类型，表明本次提交的性质
- **scope**: 可选，表明本次修改影响的范围
- **description**: 简短描述，不超过50个字符
- **body**: 可选，详细描述，说明修改的动机和与以前行为的对比
- **footer**: 可选，包含重大变更(BREAKING CHANGE)说明或关闭issue的引用

### 格式规则详解

1. **主题行**：
   - 必须使用命令式语气，如"add"而非"added"
   - 首字母不大写
   - 结尾不加句号
   - 不超过50个字符

2. **正文**：
   - 使用空行分隔主题行和正文
   - 解释"为什么"而不是"怎么做"（代码已经说明了"怎么做"）
   - 每行不超过72个字符
   - 可以使用多段落

3. **脚注**：
   - 重大变更必须在脚注中标记"BREAKING CHANGE:"或"!"
   - 引用相关的issue或PR

## 提交类型

常用的提交类型包括：

- **feat**: 新功能 (feature)
- **fix**: 错误修复 (bug fix)
- **docs**: 文档变更 (documentation)
- **style**: 代码风格变更(不影响代码功能，如格式化、缺少分号等)
- **refactor**: 代码重构(既不是新增功能，也不是修复错误)
- **perf**: 性能优化 (performance improvement)
- **test**: 测试相关变更 (adding missing tests or correcting existing tests)
- **build**: 构建系统或外部依赖变更 (build system or external dependencies)
- **ci**: 持续集成配置变更 (CI configuration files and scripts)
- **chore**: 其他变更(不修改src或测试文件)
- **revert**: 撤销之前的提交

### 类型的使用场景

- **feat**: 
  - 添加新的API或功能
  - 为现有功能添加新选项

- **fix**:
  - 修复Bug或错误
  - 更改导致不正确行为的代码

- **docs**:
  - 更新README或其他文档文件
  - 添加或修改代码注释

- **style**:
  - 修改代码格式（空格、缩进、分号）
  - 不影响代码逻辑的变更

- **refactor**:
  - 重组代码结构而不改变功能
  - 重命名变量/方法
  - 优化代码可读性

- **perf**:
  - 优化算法
  - 减少内存使用
  - 提高响应时间

## 提交示例

```
# 功能开发
feat(auth): implement user login functionality

Adds login form and authentication service to allow users to sign in with 
email and password. Includes password strength validation and remember me option.

Resolves #123

# 错误修复
fix(api): resolve null pointer when user id not found

When user ID is not found in the database, the app now returns a 404 error
instead of crashing with a null pointer exception.

Fixes #456

# 文档修改
docs(readme): update installation instructions

Update installation guide to include new environment variables required 
for authentication service.

# 代码重构
refactor(user-service): simplify authentication logic

Refactor authentication logic to use strategy pattern, making it easier
to add new authentication methods in the future.

# 包含重大变更的提交
feat(api)!: change response format of user endpoints

Change all user-related endpoints to return JSON objects with camelCase
properties instead of snake_case.

BREAKING CHANGE: User API now returns JSON objects instead of arrays.
Clients need to update their parsers accordingly.
```

## 分支管理策略

采用[Git Flow](https://nvie.com/posts/a-successful-git-branching-model/)或[GitHub Flow](https://guides.github.com/introduction/flow/)作为分支管理策略：

### Git Flow

适合有计划发布周期的大型项目。

#### 主要分支

- **master/main**: 稳定的生产版本，只合并来自release或hotfix的分支
- **develop**: 开发分支，包含最新的开发内容，所有feature分支都合并到此

#### 支持分支

- **feature/\***: 从develop分支创建，用于开发新功能，完成后合并回develop
  - 命名约定: `feature/feature-name` 或 `feature/issue-123`
  - 示例: `feature/user-authentication`

- **release/\***: 从develop分支创建，用于发布准备，完成后合并到master和develop
  - 命名约定: `release/version-number`
  - 示例: `release/1.2.0`

- **hotfix/\***: 从master分支创建，用于修复生产环境问题，完成后合并到master和develop
  - 命名约定: `hotfix/issue-description` 或 `hotfix/issue-123`
  - 示例: `hotfix/login-crash`

### GitHub Flow

适合持续部署的小型项目或网站。

- **main**: 主分支，始终包含稳定和可部署的代码
- **feature/\***: 从main分支创建功能分支，完成后通过Pull Request合并回main
  - 命名约定: `feature/feature-name` 或 `username/feature-name`
  - 示例: `feature/search-functionality` 或 `john/search-functionality`

## GitHub特定的提交规范

在GitHub上进行项目协作时，除了遵循一般的提交规范外，还应特别注意以下几点：

### Pull Request (PR) 规范

- **PR标题格式**：遵循与提交信息相同的格式 `<type>(<scope>): <description>`
- **PR描述模板**：使用组织或项目定义的PR模板，通常包括：
  - 变更概述
  - 相关issue
  - 实现方案
  - 测试方法
  - 截图（如适用）
  - 检查清单

```markdown
## 变更概述
简要描述此PR的目的和变更内容

## 相关Issue
Fixes #123

## 实现方案
简要描述实现方案和考虑的替代方案

## 测试方法
描述如何测试这些变更

## 截图
如果适用，添加截图展示变更

## 检查清单
- [ ] 代码遵循项目的编码规范
- [ ] 添加了必要的测试
- [ ] 更新了文档
- [ ] PR经过自我审查
```

### 每次提交的规范

每次提交应该是原子性的，代表一个逻辑完整的变更。具体规范如下：

1. **提交大小**
   - 每次提交应该足够小，通常不超过300-500行代码变更
   - 大型功能应拆分为多个小型、逻辑独立的提交

2. **提交内容**
   - 相关的代码更改应放在同一个提交中
   - 不相关的代码更改应分开提交
   - 格式化代码的提交应与功能变更的提交分开

3. **提交信息**
   - 第一行（主题行）：简明扼要，不超过50个字符
   - 空一行
   - 详细描述：解释为什么进行此更改，而不是如何更改
   - 引用相关issue：`Fixes #123` 或 `Relates to #123`

4. **提交时机**
   - 完成一个逻辑单元的工作后立即提交
   - 每天结束工作前提交当天的进展
   - 切换任务前提交当前任务的进展

5. **提交前检查**
   - 使用 `git diff --staged` 检查将要提交的更改
   - 确保没有包含调试代码、console.log 或 print 语句
   - 确保没有合并冲突标记（如 `<<<<<<`, `=======`, `>>>>>>>`）

### Issue 关联规范

正确关联提交和Issue对项目管理至关重要：

- **关闭Issue**：在提交信息或PR描述中使用关键词，如 `Fixes #123`, `Closes #123`, `Resolves #123`
- **引用Issue**：使用 `#123` 格式引用相关Issue，但不关闭它
- **多Issue关联**：可以在一个提交中关联多个Issue，如 `Fixes #123, #124, #125`

#### 常用关键词

以下关键词会在提交合并到默认分支后自动关闭相关Issue：

- `close`, `closes`, `closed`
- `fix`, `fixes`, `fixed`
- `resolve`, `resolves`, `resolved`

### 代码审查反馈处理

收到代码审查反馈后的提交规范：

- **修复建议**：新的提交应明确说明是针对审查反馈的修复，如 `fix(review): address PR feedback`
- **保持提交历史**：优先使用新提交而非修改历史提交（除非项目明确要求使用squash或rebase）
- **解决冲突**：当PR与目标分支发生冲突时，使用merge或rebase解决，并在提交消息中说明

## 提交最佳实践

- **原子性提交**：每个提交应该表示一个逻辑上独立的变更，便于理解、审查和必要时回滚
- **频繁提交**：避免大量改动积累后再一次性提交，这样更容易跟踪进度和定位问题
- **清晰明了**：提交信息应清晰描述做了什么改动和为什么做这些改动
- **关联问题**：在提交信息中引用相关的issue或任务，便于追踪需求来源

### 避免的反模式

- **无意义的提交信息**：如"修复bug"、"更新代码"、"WIP"等
- **大型杂乱的提交**：包含多个不相关变更的提交
- **修复提交**：如"修复上一个提交的拼写错误"
- **合并未经测试的代码**：提交前应确保代码至少通过了基本测试

## 提交前检查

在提交前，确保：

- **代码已检视**：自我审查代码变更，确保质量和一致性
- **通过测试**：确保所有相关测试通过，包括单元测试和集成测试
- **格式化代码**：使用项目规定的代码格式化工具（如black、prettier等）
- **无敏感信息**：确保没有包含密码、API密钥、证书等敏感信息
- **无调试代码**：移除临时调试代码、打印语句和注释掉的代码
- **文档更新**：如有必要，更新相关文档，包括README、API文档等

### 提交前检查清单

```
[ ] 代码符合项目的代码规范和风格指南
[ ] 所有测试都已通过
[ ] 没有包含调试代码或打印语句
[ ] 没有硬编码的敏感信息
[ ] 提交信息符合约定式提交格式
[ ] 相关文档已更新
[ ] 所有文件都是有意包含的（没有意外提交不相关文件）
```

## 使用工具辅助

使用以下工具确保提交规范：

- **commitlint**：验证提交信息是否符合约定式提交规范
  - 配置示例：`.commitlintrc.js`
  ```javascript
  module.exports = {
    extends: ['@commitlint/config-conventional'],
    rules: {
      'body-max-line-length': [2, 'always', 100],
    },
  };
  ```

- **commitizen**：交互式生成符合规范的提交信息
  - 安装：`npm install -g commitizen`
  - 使用：`git cz` 替代 `git commit`

- **husky**：设置Git钩子，在提交前自动运行检查
  - 配置示例：`.husky/commit-msg`
  ```bash
  #!/bin/sh
  . "$(dirname "$0")/_/husky.sh"
  
  npx --no -- commitlint --edit $1
  ```

- **pre-commit**：在提交前自动运行lint和测试等检查
  - 配置示例：`.pre-commit-config.yaml`
  ```yaml
  repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
    - id: trailing-whitespace
    - id: end-of-file-fixer
    - id: check-yaml
  ```

### 工具集成示例

下面是一个完整的工具链集成示例，适用于JavaScript/TypeScript项目：

1. 安装依赖：
```bash
npm install --save-dev @commitlint/cli @commitlint/config-conventional husky commitizen cz-conventional-changelog
```

2. 配置commitlint：`.commitlintrc.js`
```javascript
module.exports = {
  extends: ['@commitlint/config-conventional']
};
```

3. 配置commitizen：`package.json`
```json
{
  "scripts": {
    "commit": "cz"
  },
  "config": {
    "commitizen": {
      "path": "cz-conventional-changelog"
    }
  }
}
```

4. 配置husky：
```bash
npx husky install
npx husky add .husky/commit-msg 'npx --no -- commitlint --edit $1'
npx husky add .husky/pre-commit 'npm run lint && npm test'
```

---

遵循这些Git提交规范将有助于保持项目历史的清晰和有序，提高团队协作效率，并使问题追踪和版本管理更加简单。良好的提交习惯是专业软件开发实践的重要组成部分。 