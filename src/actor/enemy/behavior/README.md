# Enemy behavior actions

Enemy behavior is selected by `EnemyBehaviorController`. Death, knockback, and airborne handling remain in
`EnemyStateMachine` as higher-priority interrupts.

## Adding an action

1. Add a class derived from `EnemyBehaviorAction` under `behavior/actions`.
2. Implement `CanStart`, `CanContinue`, `Evaluate`, and `Update`.
3. Register the YAML type in `EnemyBehaviorActionFactory::Create`.
4. Add the action type and its parameters to a profile in `assets/data/actor/enemies.yaml`.
5. Assign that profile with `behaviorProfile` on an enemy type.

`weight` controls the relative probability when multiple actions can start in the same situation. For example,
weights of `70` and `30` select those actions with probabilities of 70% and 30%. Additional numeric YAML keys are
available through `GetParameter`.

If a profile is missing, contains an unknown action, or has no action capable of handling the current legacy state,
the controller falls back to the original idle, chase, or melee action. This keeps existing enemies operational while
new actions are being developed.
