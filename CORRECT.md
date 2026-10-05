# CORRECT — MyNameChef

Classes: (1) Non-deterministic RNG creep — replaced 6531cfe,9674e1a,0f42784,c1ba51b. Level: lint. Commit 78c5a97. Proof: random_device file in src/ -> 1; current passes.
(2) Double timing scale — 62f3f4f + 6c20cfe (scale^2, issue 65). Level: lint banning scaled dt, allowlist BattleTiming/UnifiedAnimation. Commit 910d0cb. Proof: historic systems.run(dt*scale) -> 1; current passes.
Not re-fixed: CODE_REVIEW_100_ISSUES architecture items are tracked there.
Rule table: PROJECT_RULES.md ## Correct Rules.
