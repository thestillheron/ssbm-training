# Drill history: scores over attempts, on a graph

Status: ready-for-agent

## Problem Statement

The drill summary shows one attempt's score and then it's gone. A player working through the curriculum can't see whether they're improving at a drill or how close they are to passing it reliably.

## Solution

Each curriculum drill keeps its [history](../../docs/glossary.md#glossary_history): the [score](../../docs/glossary.md#glossary_score) of every recorded [attempt](../../docs/glossary.md#glossary_attempt), saved to the training build's own save ([own-game-id](../own-game-id/spec.md)). X on a drill in the event list opens its [history graph](../../docs/glossary.md#glossary_history_graph): attempt number across, score up, and a line at the pass mark.

## Implementation Decisions

- **What is recorded.** An attempt is recorded when its drill summary opens. Leaving early (Restart drill, Change characters, Back to event list, or Edit layout restarting the drill) records nothing; Restart from the summary starts a new attempt. Not recorded: the Custom drill, an attempt played with an edited layout (a session copy that differs from the table entry), and test drills in authoring builds.
- **Stable drill IDs.** Each drill record in the drill table carries a stable ID that history is keyed by, not its table index, so reordering, inserting or retuning drills (pass mark included) keeps history. A removed drill's history is orphaned.
- **Storage.** Per drill: the latest 100 scores (one byte each) and a running total of attempts, so the x axis shows real attempt numbers (e.g. 401 to 500) once older scores drop off. Kept in the training build's save data.
- **Saving.** On recording, the attempt is added in RAM and `lbCardGame_SaveChanges` is called, which marks the save dirty for Melee's background write; no notice is shown. With no card or no save (the player declined it), history lives in RAM for the session and the graph still shows it. A failed write never affects the summary.
- **Graph screen.** X on a curriculum drill's event in the event list opens the graph full screen; B or X closes it, back on the same drill. It shows the drill name and total attempts; the y axis labelled at 0, the pass mark and the top; the x axis labelled at its first and last attempt numbers; scores as a line with a dot per attempt; and the pass-mark line in a contrasting colour. A drill without a pass mark has no line. With no attempts it shows "No attempts yet" instead.
- **Y scale.** A rep-count drill: 0 to its length. A timed drill: 0 to its best score so far, rounded up.
- **Hint.** The event list's description panel shows "X: History" on curriculum drills, not on Custom. X is unused on the event list today (`mnEvent_8024D864`).

## Testing Decisions

- A scenario plays a short drill to its summary twice, returns to the event list and opens the graph: it shows two attempts with the summary's scores and the pass-mark line (screenshot).
- A scenario that restarts mid-drill, then finishes, shows one attempt.
- History survives a power cycle: a second run against the run copy left by the first shows the earlier attempts (or a test reads them back from the save data).

## Out of Scope

- Dates or session grouping on the x axis.
- History for the Custom drill or edited layouts.
- Per-tech-option history, timing breakdowns, and clearing history.
