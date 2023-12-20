# A asychronous pipelined graph executor.

A producer-consumer model based asychronous graph executor that targets at
parallelsim at:
- Per node level: Different nodes inside the same run can execute
  concurrently.
- Per run level: Different nodes inside the different runs can execute
  concurrently.

Example:

```text
       -> B (2s)
A (1s) -> C (3s) -> E (5s)
       -> D (4s)
```

For 10 executions:

- Full sequantial:
  - Requires (1 + 2 + 3 + 4 + 5 ) * 10 = 150s
- Asynchronous parallel execution inside same run:
  - Requires (1 + max(2, 3, 4) + 5) * 10 = 100s
- Asynchronous parallel execution across different runs:
  - Requires (1 + max(2, 3, 4) + 5) + max(1, 2, 3, 4, 5) * 9 = 55s
