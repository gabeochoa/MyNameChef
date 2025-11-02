# My Name Chef Project Rules

## Git Commit Format
Use short, descriptive commit messages with prefixes:
- no prefix for new features
- `bf -` for bug fixes  
- `be -` for backend/engine changes or refactoring
- `ui -` for UI changes

Rules:
- Use all lowercase
- Avoid special characters like ampersands (&) - use "and" instead
- Keep messages concise and descriptive

Examples:
- `implement proper kill attribution system`
- `bf - fix UI not showing after game ends`
- `be - combine RenderHealthAndLives and RenderKills into unified RenderPlayerHUD`

## Code Style
- Keep functions focused and single-purpose
- Prefer early returns to reduce nesting
- Dont add any comments unless explicitly asked 
- use a function instead of a line with multiple ternary expressions
- Avoid using `auto` for non-template types - use explicit types instead
- Use `for (Entity &entity : EntityQuery().gen())` instead of `for (auto &ref : ...)` with `ref.get()` - the range-for will auto-extract the entity from the reference wrapper
- Prefer references over pointers when possible - when iterating over containers of pointers, dereference immediately: `for (Type *ptr : container) { Type &ref = *ptr; ... }` instead of using `ptr->` throughout

## Project Structure
- `src/` contains main game code
- `vendor/` contains third-party libraries
- `resources/` contains assets (images, sounds, fonts)
- `output/` contains build artifacts

## Build System
- Use `xmake` to build project
- Game executable is `my_name_chef.exe`

## Debugging
- Use `log_info()`, `log_warn()`, `log_error()` for logging
- Add debug logs for complex systems like damage tracking
- Remove verbose debug logs before committing

## TODO Comments
- Use `TODO` for incomplete features or known issues
- Use `TODO` for performance optimizations (e.g., "TODO this allocates...")
- Use `TODO` for future improvements (e.g., "TODO add random generator")

## Component Patterns
- All components inherit from `BaseComponent`
- Use `struct` for components, not `class`
- Components should be simple data containers
- Use `std::optional` for nullable fields
- Use `enum class` for component state enums

## System Patterns
- Systems inherit from `System<Components...>`
- Override `for_each_with(Entity&, Components&..., float)` for main logic
- Use `virtual void for_each_with(...) override` syntax
- Systems should be focused and single-purpose
- Use early returns to reduce nesting

## Singleton Patterns
- Use `SINGLETON_FWD(Type)` and `SINGLETON(Type)` macros
- Singletons should have `get()` static method
- Use `SINGLETON_PARAM` for parameterized singletons
- Delete copy constructor and assignment operator

## Enum Patterns
- Use `enum struct` for type safety
- Use `magic_enum` for enum utilities
- Use `magic_enum::enum_names<EnumType>()` for string lists
- Use `magic_enum::enum_count<EnumType>()` for count

## Naming Conventions
- Use `camelCase` or snake_case for variables and functions
- Use `PascalCase` for structs, classes, and enums
- Use `UPPER_CASE` for constants and macros
- Use descriptive names that indicate purpose
- Use `has_` prefix for boolean components (e.g., `HasHealth`)
- Use `Can_` prefix for capability components (e.g., `CanDamage`)

## File Organization
- Use `#pragma once` at top of header files
- Group includes: standard library, third-party, project headers
- Use forward declarations when possible
- Keep header files focused and minimal

## HUD and UI Systems
- Create focused HUD systems for specific data (e.g., `RenderWalletHUD` not `RenderEverythingHUD`)
- Use singleton access for global game state (wallet, health, etc.)
- Always check for singleton existence before rendering

## Shop and Economy Patterns
- Use singleton components for global game state (Wallet, Health, ShopState)
- Register singletons in manager function (e.g., `make_shop_manager`)
- Use descriptive component names that indicate purpose
- Store intrinsic properties in the most logical component 
- Use `EntityHelper::get_singleton<Type>()` for singleton access
- Handle singleton existence checks before accessing components

## Query and Filtering Patterns
- Prefer `EntityQuery` when possible over manual entity iteration
- Use `whereLambda` for complex filtering conditions
- Use `orderByLambda` for sorting entities instead of `std::sort`
- Chain multiple `where` clauses for better performance
- Use `gen_first()` for finding single entities instead of loops
- Extract complex query logic into helper functions for reusability
- Use `RefEntities` (from query results) instead of manually creating vectors of references

## Component Design Principles
- Prefer pure tag components over components with member variables
- Use composition over configuration (multiple small components vs one large component)
- Each component should have a single, clear responsibility
- Use separate components for data instead of boolean flags (e.g., `IsDisabled` instead of `enabled` field)
- Tag components should have no members: `struct IsDraggable : BaseComponent {};`
- Data components should contain only related data: `struct DragOffset : BaseComponent { vec2 offset; };`

## System File Naming
- All system header files must use UpperCamelCase naming convention
- Examples: `MarkIsHeldWhenHeld.h`, `DropWhenNoLongerHeld.h`, `RenderPlayerHUD.h`
- Update include statements when renaming system files
- Check for internal includes within system files that reference other systems

## Draggable Item System Patterns
- Use `IsDraggable` as pure tag component for draggable entities
- Use `IsHeld` as pure tag component for currently held entities
- Use `IsDropSlot` as pure tag component for valid drop targets
- Use `CanDropOnto` as pure tag component for entities that accept drops
- Store drag data in separate components: `DragOffset`, `OriginalPosition`
- Use `IsOccupied` tag component instead of boolean `occupied` field
- Use `AcceptsInventoryItems` and `AcceptsShopItems` tag components instead of boolean fields
- Systems should query for component presence/absence rather than checking member values

## Entity State Management
- When swapping entities, update all related slot states immediately
- Ensure slot `occupied` states are properly synchronized with item positions
- Use `EntityQuery().whereHasComponent<Component>()` for filtering instead of manual loops
- Always check for component existence before accessing: `if (entity.has<Component>())`

## Cross-System Communication
- Use component presence/absence for inter-system communication
- Avoid direct system-to-system dependencies
- Use `EntityHelper::merge_entity_arrays()` when creating entities that other systems need to query
- Ensure entity creation order matches rendering order (slots before items)

## Testing Framework
- Use `--run-test <test_name>` flag to execute individual tests
- Tests run within the main game loop using `TestSystem`
- Each test is a separate `.h` file in `src/testing/tests/`
- Tests can validate UI elements, entities, and game state
- Use `UITestHelpers` for UI interaction and validation
- Use `TestInteraction` for safe game state mutations

### Test Categories
- **Navigation Tests**: Validate screen transitions (Main → Shop → Battle)
- **System Tests**: Validate internal game systems (dish, combat, trigger)
- **UI Tests**: Validate UI elements and interactions
- **Integration Tests**: Validate complete game flows

### Running Tests
- Individual test: `./output/my_name_chef.exe --run-test <test_name>`
- All tests: `./scripts/run_all_tests.sh`
- Test timeout: 5 seconds per test (prevents infinite loops)

### Pre-Commit Test Requirements
- **All tests must pass in BOTH headless and non-headless modes before committing**
- Run headless tests: `./scripts/run_all_tests.sh` (default headless mode)
- Run visible tests: `./scripts/run_all_tests.sh -v` (non-headless mode with visible windows)
- Both test runs must show "🎉 All tests passed!" before committing
- If tests fail in one mode but pass in the other, fix the issues before committing

### Test Development Guidelines
- Tests should be self-contained and not depend on external state
- Use `// TODO` comments to document expected behavior and bugs
- Tests should identify bugs, not just validate working features
- Prefer entity validation over UI validation when possible
- Add `// TODO` comments in game code for UI label improvements
- **Never use `GameStateManager::get().update_screen()` or directly manipulate game state in tests**
- **Always use UI interactions (clicks, waits) to navigate between screens - click buttons to change screens, never directly set screen state**
- **Avoid branching logic in tests - use `wait_for_ui_exists()` and `click()` to navigate, let waits handle timing**
- **Avoid if/else branches in tests - use assertions (`expect_true`, `expect_false`, `expect_count_eq`, etc.) to validate conditions**
- **For cases that seem to require branching, create separate test functions with different assertions and fail conditions to improve coverage**
- Tests will timeout after 1 second if waiting for a condition that never completes
- Use `create_inventory_item()` or similar helper functions to set up test state rather than checking if conditions exist and branching

## Refactoring and Development Workflow
- Extract helper functions to reduce code duplication
- Use consistent patterns across similar systems
- Update related systems when changing component structures
- Test builds after each major refactoring step
- Run `xmake` after each significant change to catch compilation errors
- Fix build errors immediately before continuing development
- Include all necessary component headers in system files
- Use early returns for error conditions in systems
- Run tests after major changes to ensure no regressions
