# Theme / Metaphor Decisions — Draft

Status: Draft (interview in progress; only explicit user acceptance makes this Final)

Purpose: settle what My Name Chef is *about* — the metaphor that explains who
the player is, who the opponent is, who judges, and what winning a battle
means. The mechanics (shop → build a team of dishes → course-by-course
survivor battles vs a server-matched opponent → verified results) are fixed;
this document settles the story wrapped around them.

Candidate metaphors already mocked in `game_mock.html` (version dropdown):
classic bistro, Michelin/executive chef, Flavortown (diners/drive-ins/dives),
alien DLC (Galactic Galley), cooking-show competition (Chef Showdown).

## Settled decisions

- Process decision (user, earlier this session): do NOT pick a metaphor yet.
  Keep all five candidate themes (classic, michelin, flavortown, alien,
  showdown) in the mock, each expanded with a full briefing (fantasy,
  opponent, judge, win/lose stakes, tone, sample review quotes) so the user
  can compare them before deciding. Q1–Q3 are deferred until then.
- Narrowing (user, this session): favorites were **Michelin Star** and
  **Chef Showdown**. Classic, Flavortown, and Alien are deprioritized (kept
  in the mock as alternates/DLC flavor, not the core identity).
- **DECISION (user, this session): primary metaphor is Michelin Star.**
  The player is an executive chef chasing stars; the judge is the anonymous
  inspector whose verdict the player never controls or argues with.
  Chef Showdown is the runner-up (possible alternate mode / DLC framing,
  not the core identity).
- Noted convergence that led here: both favorites have an explicit JUDGE.
  Classic has none, Flavortown's judge loves everything (no stakes),
  Alien's is a gag. Michelin + Showdown both frame cooking as performance
  that someone else evaluates — which matches the mechanics: the player
  composes, then watches, and does not control the battle itself.
- World-thesis candidates discussed (rating culture / who decides what's
  good / substance vs spectacle / passion-economy burnout / judgment as
  honest craft feedback / no thesis). The Michelin pick is settled; which
  specific thesis, if any, the game leans into is NOT yet pinned down.
- Brand risk flagged (user, this session): "Michelin" is a trademark.
  Using it as game branding/theme risks a cease-and-desist even if a
  lawsuit is unlikely; descriptive/editorial mention is lower risk than
  title/store/logo use. Likely workaround: a fictional guide with the
  same metaphor (anonymous inspectors, stars, a feared guidebook) under
  an invented name. Unresolved — see Q24. Not legal advice.
- **DECISION (user, this session): rename to a fictional award.** Working
  name proposed by user: **the Golden Spoon** — the guide's top honor,
  with the defining trait that it is *easy to lose* ("you can lose your
  golden spoon very easily"). Design note: the award is lent, never
  owned — reputation fragility is part of the metaphor, not just its
  decoration.
- **DECISION (user, this session): award is the Silver Cloche**, chosen
  over Golden Spoon. Rationale (user): the cloche silhouette is nice —
  a big icon that is easy to see/read at small sizes. The fragility
  trait carries over: a cloche is easy to lose. Publisher fit: a
  silverware/metalware company makes cloches as readily as spoons, so
  the leading publisher candidate below is unaffected.
- Guide-publisher origin (under discussion): modeled on the real Michelin
  logic — the Guide exists because a tire company profited when people
  drove to restaurants. The fictional publisher should likewise profit
  when others eat out, while posing as an objective food authority.
  Leading candidate: a cutlery/silverware company (the award is its own
  flagship spoon; dining boom = flatware sales). Alternates: linen
  supplier, carriage/fuel company (direct tire homage), antacid/tonic
  maker (comedy), reservations platform (modern/rating-culture lean).
  Payload: the world's top food authority does not care about food —
  chefs chase a verdict issued to sell a product.
- **DECISION (user, this session): publisher is a silverware company.**
  The guide that awards Silver Cloches is published by a silverware /
  metalware firm as marketing, on the Michelin-tire model: more
  restaurants chasing cloches means more dining rooms buying its
  flatware and serving pieces, and every award is its own merchandise
  displayed in a rival's dining room.
- Follow-up requested (user, this session): redesign the mock UI to be
  cloche-themed (see `game_mock.html`).
- **DIRECTION CHANGE (user, this session):** the first cloche redesign
  reads as boring/plain, not fun. The other mock themes (classic,
  flavortown, alien, showdown) are DELETED from the mock. Exploration
  restarts from real restaurants: 25 variants styled on the World's 50
  Best 2025 top 25 (Maido #1 through Odette #25), each borrowing that
  room's palette/type/character, so the user can pick what feels fun.
  Two variants (DiverXO, Gaggan) are rendered in GBA pixel style, since
  the food art is pixel art and the UI may want to match it. The
  story/brief card is removed from the mock page. The Silver Cloche
  metaphor itself is unchanged; this is about the visual language.
- **RESTRUCTURE (user, this session):** user asked how different the 25
  really were vs colorways. Honest audit: all differed in palette and
  texture, most in typeface/radius, but they shared one layout and only
  ~5 structural treatments. Fix: the mock now splits two independent
  axes — **Style** (Modern / GBA / DS / PS1: structural era treatments —
  pixel type + chunky borders + dither; pill touch buttons + two-tone
  cards + hinge hardware + VT323; gradient window panels + bevels +
  scanlines + title jitter) × **Palette** (the 25 restaurants, colors /
  type / texture only). 4 × 25 = 100 combinations, two dropdowns.
- **STYLE SHELF EXPANDED (user, this session):** user likes PS1. Added
  PS2 (gloss + glow), Sega Saturn (beveled gray console chrome, dark
  engraved text, checkerboard floor), Dreamcast (console-white panels).
  7 styles × 25 palettes = 175. Also fixed a model leak: DiverXO/Gaggan
  palettes carried pixel fonts; pixel type now belongs only to styles.
- **FULLSCREEN LAYOUT (user, this session):** mock stage now fills the
  viewport under a slim top bar (was a fixed 1280×720 box with content
  in the top half). Menu gained a "Tonight / inspector due" standing
  panel; battle gained a 7-dot course tracker and a centered arena;
  shop rows distribute over the full height; results centers its list.
- **CODEX REVIEW ROUND (user, this session):** screenshots sent to
  Codex; it ranked GBA > DS > PS1 > Dreamcast > PS2 > Saturn > Modern
  and flagged: DS hinge occluding cards (critical), scattered menu
  focal points, glow/dot overuse in PS2, Saturn reading as OS widgets,
  Dreamcast rings competing, Modern having no identity. Fixes applied:
  DS battle contained in the top screen with tracker/legend on the
  bottom screen; Quit demoted into the menu's meta group; premise line
  shortened; PS2 dots dimmed, card interiors made opaque, one white
  selection glow, shop rows spread full-width; Saturn rebuilt as 2D
  arcade (bitmap bands, checker floor removed, shell relabeled);
  Dreamcast rings reduced to a corner swirl, sans controls, grouped
  selector panel; Modern hatch removed, left-aligned asymmetric menu,
  sans controls; PS1 menu became one big window box with bitmap title;
  GBA cloche placed in an icon plate, TONIGHT panel squared.
- **FINAL THREE (user, this session):** user likes DS book mode most,
  GBA second (but "doesn't feel GBA enough"), PS1 third (likes menu +
  movement; food didn't match). Modern, PS2, Saturn, Dreamcast are
  REMOVED from the mock (dropdown and CSS). DS is now the default.
  GBA authenticity pass: indigo handheld body, D-pad and A/B button
  silhouettes in the side margins (wide viewports), fine 4px dither,
  LCD line-grid overlay, inner keylines on windows. PS1 food pass:
  sprites desaturated, slightly blurred, drop-shadowed, and given the
  affine wobble on every screen so the food matches the jittery world.

## Open questions (25)

1. Core fantasy: who is the player?
2. Who is the opponent, really?
3. Who judges — who decides the winner means something?
4. What does a "battle" depict, in-fiction?
5. What is a round?
6. What does losing health mean?
7. What is gold/currency, in-fiction?
8. What is the shop, in-fiction?
9. Why do two identical dishes merge into a stronger one?
10. What do cuisine tags / set bonuses represent?
11. What is the rival's team (the server pool of other players' menus)?
12. What does the house team (fallback opponent) represent?
13. Tone: cozy, cutthroat, comedic, prestige — pick a center of gravity.
14. The title: does "My Name Chef" stay? What does the name promise?
15. Results screen: review, scorecard, verdict, headline — what artifact?
16. Progression across rounds: a night, a season, a career, a road trip?
17. Lose condition: what happens when health hits zero, in-fiction?
18. Win condition: is there a final victory, or endless climb?
19. The player-facing fantasy verb: cook, compete, impress, survive, explore?
20. Audience surrogate: customers, critics, judges, fans — who reacts on screen?
21. Drinks/desserts: what role in the fiction (pairings, courses, sponsors)?
22. Cuisines as expansions: are cuisine packs the DLC axis?
23. How much story text per battle: none, one-liners, full review paragraphs?
24. Real-world references (Michelin, Flavortown): homage, parody, or avoid brands?
25. One-sentence pitch: what do we tell a stranger the game is?
