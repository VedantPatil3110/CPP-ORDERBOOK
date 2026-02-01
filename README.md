# C++  Order Book

A single-threaded **price–time priority limit order book** implemented in C++.

This project simulates the core logic of a trading exchange, handling order matching,
execution, cancellation, and different order lifecycles.

---

## 🚀 Features

- Buy & Sell **limit orders**
- Order types:
  - Good Till Cancel (GTC)
  - Good For Day (GFD)
  - Fill And Kill (FAK)
  - Fill Or Kill (FOK)
- **Partial and full order fills**
- Price–time priority matching
- Order modification (cancel + reinsert)
- End-of-day cleanup for GFD orders

---

## 🧠 Core Concepts Implemented

- Best bid / best ask matching
- FIFO matching at each price level
- Order lifecycle management
- Trade generation between bid and ask
- Safe iterator handling during matching and cancellation

---

## 🧱 Data Structures Used

- `map<Price, list<Order>>`
  - Ordered price levels
  - Descending for bids, ascending for asks
- `unordered_map<OrderId, OrderEntry>`
  - O(1) order lookup and cancellation
- `list`
  - Preserves time priority (FIFO)

---
