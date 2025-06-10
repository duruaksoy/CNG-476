# CNG-476

## Project Overview

This project simulates an IoT-based **Smart Small Grid Management System** using **LoRa communication** within the **OmNet++** simulation environment and the **FLoRa framework**. The goal is to model a responsive and scalable grid infrastructure that detects power outages in real time, manages alert messages using queuing mechanisms, and evaluates system responsiveness under uncertainty.

Each smart house in the system is equipped with a sensor node that continuously monitors power availability. When an outage is detected, a LoRa message is sent to a central monitoring system via a LoRa Gateway and Network Server. The monitoring system queues incoming outage reports using a **FIFO (First-In-First-Out)** structure and forwards them to the technical response team for resolution.

To simulate real-world conditions and uncertainties such as unpredictable outages or sensor failures, the system employs **Monte Carlo simulation**, **Poisson processes**, and **random number generation**. The simulation measures key performance indicators such as detection accuracy, processing time, and resolution efficiency to assess the effectiveness of the proposed grid model.

---

## Technologies Used

- **Programming Languages**:
  - C++ (for simulation logic in OmNet++)
  - NED (Network Description Language for OmNet++ topologies)
  - LaTeX (for documentation and reporting)

- **Simulation Tools**:
  - [OmNet++](https://omnetpp.org) – A modular, component-based C++ simulation framework for discrete event simulations
  - [FLoRa Framework](https://github.com/katharinalab/FLoRa) – A simulation framework for LoRaWAN built on OmNet++

- **Mathematical and Modeling Techniques**:
  - **Poisson Process** – Models the arrival of power outage events to emulate real-world randomness
  - **Monte Carlo Simulation** – Introduces variability in the network to evaluate robustness
  - **Queueing Theory (FIFO Queues)** – Handles and prioritizes outage reports based on arrival time
  - **Random Number Generation** – Simulates stochastic behavior in outage detection and transmission delays

---

## Performance Metrics

- Number of outages detected by each smart house
- Average response and processing time of the system
- Total processing time for reported outages

---
