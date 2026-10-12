# 02: Line through the scores

**What to build:** the history graph joins each attempt's dot to the next with a line, drawn above the graph's background and below its dots. It is the training build's first drawing beyond text and axis-aligned rectangles, so it may want a small reusable line-drawing helper ([spec](../spec.md)).

**Blocked by:** 01 (History graph for this session)

**Status:** ready-for-agent

- [ ] With three or more attempts whose scores rise and fall, the graph shows a connected line through every dot in attempt order (screenshot).
- [ ] With one attempt there is a dot and no line.
- [ ] The line is gone once the graph closes and draws nothing in a match.
