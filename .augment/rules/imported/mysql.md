---
type: "agent_requested"
---

# MySQL rules

- Use appropriate data types to optimize storage and performance (e.g., INT for IDs, VARCHAR with appropriate length)
- Create indexes for columns used in WHERE, JOIN, and ORDER BY clauses
- Use foreign keys to maintain referential integrity
- Use EXPLAIN to analyze and optimize queries
- Avoid using SELECT * and only retrieve needed columns
- Use prepared statements to prevent SQL injection
- Use appropriate character set and collation (e.g., utf8mb4_unicode_ci)
- Use transactions for operations that must be atomic
