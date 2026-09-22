# Project Deep Dive: S.N. Engineering Works — Full Stack Developer Intern

This project relies more heavily on personal recall than the other three — the resume states outcomes ("blank repo to live production," "replaced a manual quoting workflow"), not mechanisms. This file is a question-preparation framework, not an architecture reconstruction — **you need to fill in the actual answers from memory, and everything here marked `NEEDS VERIFICATION` should be answered from your own recollection, not invented.**

---

## What the Resume States (The Only Ground Truth)

- Full Stack Developer Intern, Jul 2025 – Aug 2025
- Took a B2B machinery marketplace (React.js + Supabase) from a blank repository to a live production site, owning the product end to end
- Replaced a manual, email-based quoting workflow with a custom project configurator backed by a Supabase lead pipeline with status tracking
- Shipped an additional internal ticketing module *after* the internship ended, at the company's request

---

## Question Bank (Answer From Memory Before Your Interview — Write These Down)

### Ownership & Scope
- "What did you personally build, versus what existed already?" — Since you went from a *blank* repository, this should be a genuinely complete answer covering the full stack, not just a UI layer.
- "How did you prioritize what to build first, given you owned it end to end?"
- "What would you improve now, with more experience?" — A strong, self-aware answer here (not just "nothing, it was perfect") is a good behavioral signal.

### Technical — Supabase
- "Why Supabase specifically, for this project?" (Likely reasons to consider: fast to stand up, Postgres underneath giving relational modeling, built-in auth, generous free tier for a small B2B tool — but confirm this was actually your/the team's reasoning, don't invent a justification you didn't have.)
- "How did the database schema work?" — Draw it from memory: what tables, what relationships, especially around the lead pipeline and its status tracking.
- "How did authentication work?" — Supabase Auth directly, or something custom on top?
- "Row-Level Security (RLS)?" — Supabase's Postgres-native RLS is the standard way to scope data access per user/role in a Supabase app; `NEEDS VERIFICATION` whether you used it, and if so, how it was configured — a strong, specific answer if you did.

### Technical — Frontend
- "How did you structure the React frontend?" — Component organization, state management approach (see `05_FRONTEND/REACT.md` for the vocabulary to describe this precisely: controlled forms, lifted state, Context usage).
- "How did the quoting workflow work before and after your change?" — This is your strongest "impact" story; be ready to describe the *before* state concretely (manual, email-based — what did that actually look like operationally) and the *after* state (the configurator + lead pipeline) with equal concreteness.
- "How did you handle form validation in the configurator?" — Controlled inputs + validation logic, per `05_FRONTEND/REACT.md` Cluster 3.

### Technical — Deployment & Operations
- "How did you deploy it to production?" (Vercel/Netlify/other — and what did your deploy pipeline actually look like, manual or automated?)
- "What was the hardest bug you hit, and how did you find it?" — This is a classic, near-certain behavioral-technical hybrid question; prepare a real, specific story using the debugging framework in `15_INTERVIEW_PREP/DEBUGGING.md` (once generated) — form a hypothesis, verify it, fix it, and what you learned.

### Stakeholder & Post-Internship Ownership
- "How did you work with non-technical stakeholders at the company?" — Requirements gathering for the configurator specifically likely required translating a real, messy business process (the manual quoting workflow) into a structured data model and UI — a strong story about that translation process is valuable.
- "Why did you build the ticketing module after the internship ended?" — This is explicitly framed on your resume as a demonstration of ownership beyond scope; be ready to describe what prompted the request and why you said yes — a genuine, specific answer is far stronger than a generic "I wanted to help."

---

## A Note on This Project's Interview Weight

Given your other three projects are unusually advanced (Event Sourcing/CQRS, lock-free systems programming, eBPF), this internship often ends up interpreted differently depending on the interviewer: some will treat it as your "real-world, ship-it, deal-with-a-business" credential — proof you can operate in a messy, non-greenfield, stakeholder-driven environment, which the other three (personal/academic projects) can't fully demonstrate on their own. Lean into that framing rather than downplaying this project as the "simple" one next to your systems work — end-to-end ownership of a live production tool used by real customers is a genuinely different and valuable kind of evidence.

### Quick Revision
- This file's job is prompting you to recall real answers — every technical specific here must come from your memory, not be invented under interview pressure.
- The quoting-workflow before/after story is your strongest, most concrete impact narrative — rehearse it with specifics.
- Frame this project as complementary evidence (real-world, stakeholder-driven delivery) to your systems projects, not as the "lesser" one.
