# 03: History saved to the card

**What to build:** drill history outlives the session. Per drill, the latest 100 scores and a running total of attempts are kept in the training build's own save (`GTCE01`, so never the player's vanilla save), so once older scores drop off the x axis shows real attempt numbers (e.g. 401 to 500). Recording an attempt marks the save as changed for Melee's background write, with no notice. With no card, or a declined save, history stays session-only as in 01, and a failed write never affects the drill summary ([spec](../spec.md)).

**Blocked by:** 01 (History graph for this session); own-game-id 02 (Training build runs as GTCE01)

**Status:** ready-for-agent

- [ ] A scenario records attempts, and a second run against the run copy the first left behind shows them on the graph.
- [ ] After more than 100 attempts at one drill, the graph shows the latest 100, numbered from the running total.
- [ ] Booting with no card, or declining the save, still records and graphs this session's attempts.
- [ ] The vanilla GALE01 save on the same card is untouched.
