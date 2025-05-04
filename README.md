# CNG-476

## 📘 Project Overview

This project simulates an IoT-based **Smart Small Grid Management System** using **LoRa communication** within the **OmNet++** simulation environment and the **FLoRa framework**. The goal is to create a realistic model of a small-scale smart grid that efficiently manages power outage detection and recovery through sensor-based monitoring and queue-driven event handling.

Each house in the system is equipped with smart sensors that detect power outages and report them to a central monitoring system using LoRa’s low-power, long-range communication. The central system evaluates outages, handles them using a FIFO queue, and coordinates technical teams for resolution, simulating both sensor and network uncertainties with stochastic methods.

---

## 🛠️ Technologies Used

- **Programming Languages**: C++ (for OmNet++), NED language (for network definitions), LaTeX (for documentation)
- **Simulation Tools**:
  - [OmNet++](https://omnetpp.org) – Discrete Event Simulation Framework
  - [FLoRa Framework](https://github.com/katharinalab/FLoRa) – LoRaWAN simulation in OmNet++
- **Mathematical Techniques**:
  - **Poisson Process** – To simulate outage occurrence
  - **Monte Carlo Simulation** – For modeling uncertainties in network and sensor behavior
  - **Queueing Theory** – FIFO queues for outage handling
  - **Random Number Generation** – For event and failure simulation

---

