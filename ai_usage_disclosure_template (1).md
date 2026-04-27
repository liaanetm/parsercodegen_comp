# AI Usage Disclosure Details

**Student Name:** Lianet Martin
**Student ID:** 5636501
**Assignment:** Parser Code Generation Complete Version/ HW4

---

## Instructions

Complete this template with detailed information about your AI usage. Submit this file along with your signed PDF declaration form. Include the conversation log in order, with each round listing the prompt, AI output, and verification/edits.

---

## AI Tool #1

### Tool Name
Claude 

### Version/Model
opus 4.6

### Date(s) Used
April 15, 2026 - April 20, 2026

### Specific Parts of Assignment
I used AI assistance to understand and confirm the logic behind lexical nesting levels (L values) in code generation for a compiler project. Specifically, I asked for conceptual clarification on:
How to compute L values based on the difference between the nesting level of the reference site and the declaration site of a symbol.
How that logic applies inside the ident handling portion of the parser/code generator (variable reference and symbol lookup logic).
How the addition of procedure declarations changes the structure of the compiler (symbol table entries, level handling, and scope rules).
### Conversation Log
[Use one block per round. Add more rounds as needed.]

#### Round 1
**Prompt:** explain the logic, your generator must correctly produce L
values that reflect the difference in nesting level between the reference site and the declaration
site of each symbol,

**AI Output:** 
This is about how closures and nested scopes work in compilers/interpreters, specifically regarding static links or display levels — the L value represents how many scope boundaries you need to cross to reach where a variable was declared.

The core idea is straightforward. Every time you define a function inside another function, you create a new nesting level. When code inside an inner function references a variable declared in an outer function, the compiler needs to know how far out to look. That distance is L.

How to compute it: when you encounter a reference to a symbol, you take the current nesting depth (where the reference happens) and subtract the nesting depth where that symbol was declared.

L = depth_of_reference - depth_of_declaration
For example:

function A()          -- depth 0
  var x = 10
  function B()        -- depth 1
    function C()      -- depth 2
      print(x)        -- reference to x here
At the print(x) line, the reference site is at depth 2, and x was declared at depth 0, so L = 2 - 0 = 2. This tells the runtime to follow 2 static links up the chain of activation records to find x.

A few key cases to internalize:

L = 0 means the variable is local — declared in the same scope where it's used. No link traversal needed; you access it directly in the current activation record.

L = 1 means the variable lives in the immediately enclosing function. You follow one static link.

L > 1 means you cross multiple scope boundaries, following that many static links.

Why this matters at runtime: each activation record (stack frame) stores a pointer to the activation record of its lexically enclosing function (the static link). To access a variable with a given L, the generated code emits instructions that follow exactly L static links, then use the offset within that frame to grab the value. If your generator produces a wrong L, the code will chase the wrong number of links and read garbage or the wrong variable entirely.

A common mistake is confusing dynamic depth (call stack depth, which depends on runtime behavior) with static/lexical depth (which is fixed at compile time by looking at the source code structure). The L value is always computed from the lexical nesting, never from how functions happen to call each other at runtime.


**Verification/Edits:** I verified this explanation by comparing it to the lecture notes and the assignment specification describing static scoping. I confirmed that the L value represents the number of static links that must be followed and that it depends on lexical depth, not runtime call depth. No edits were needed.

#### Round 2
**Prompt:** so it would be current L - L from the function L
**AI Output:** Yes, exactly. You're comparing two L values that you already have stored:

The current function's nesting level (the one you're compiling right now)
The nesting level recorded in the symbol table for the variable you're referencing

The difference between those two gives you the number of static links to follow.
Say your symbol table tracks each symbol with its declaration level. When you're generating code inside a function at level 3 and you encounter a variable that was declared at level 1:
L = current_level - symbol_level
L = 3 - 1 = 2
So you emit code that follows 2 static links.
The nice thing is you don't need to re-analyze the nesting structure each time — you just look up what's already stored. Your compiler already knows its own current depth (you increment it every time you enter a function, decrement when you leave), and the symbol table already recorded each variable's depth when it was declared. It's just a subtraction at the point of reference.
If you get L = 0, the variable is local, so you access it directly in the current frame. If you get a negative number, something is wrong — that would mean you're trying to reference a variable declared in a deeper scope, which shouldn't be possible lexically.You said: explain what should I do in ident , don't give me the code
**Verification/Edits:** I checked this logic against the symbol table format used in my assignment and confirmed it matches how the CAL/LOD/STO instructions should compute L. I also validated that negative L values would indicate an invalid scope reference.

#### Round 3 (optional)
**Prompt:** for this assigment the only new function is procedure declaration
**AI Output:** Yes, essentially. The structural changes are:
One new function: PROCEDURE-DECLARATION, which parses the procedure ident ; block ; pattern, adds the procedure to the symbol table as kind 3, recurses into BLOCK with level + 1, and emits RTN after the block finishes.
One new branch in STATEMENT: The callsym case, which is just a few lines — look up the identifier, check it's kind 3, emit CAL with the computed L and the procedure's stored address.
Everything else is modifications to existing code:

BLOCK now calls PROCEDURE-DECLARATION between VAR-DECLARATION and STATEMENT, and needs to emit a JMP at the start to skip over procedure bodies, then patch it afterward
BLOCK needs to handle the level parameter — you pass level + 1 when recursing into a procedure's block
Your symbol table insert functions now need to store the correct level instead of always 0
Every place you emit LOD, STO, or CAL, you compute L = currentLevel - symbol.level instead of hardcoding 0
When a block finishes, you mark all variables and procedures declared at that level so they're out of scope
Your symbol table lookup needs to skip marked entries (mark == 1)

So it's not a huge amount of new code — it's mostly threading the level through what you already have, adding the procedure declaration loop, adding the call branch, and implementing marking.
**Verification/Edits:** I compared this explanation with the assignment rubric and confirmed that procedure declaration is the major new feature. I used the output as a checklist to ensure I updated the correct areas (BLOCK logic, symbol table levels, marking out-of-scope symbols, and CAL generation)


### Learning & Reflection
This interaction helped me understand the purpose of lexical nesting levels and how they relate to static scoping. I learned that the value L is determined purely by the structure of the source code
I also improved my understanding of how procedure declarations affect compilation, particularly the need to update the symbol table with procedure entries, increment nesting level when entering a procedure block, and ensure correct marking of symbols leaving scope.
---

## AI Tool #2 (if applicable)

### Tool Name
[Tool name if you used a different AI tool]

### Version/Model
[Version/model information]

### Date(s) Used
[Dates when this tool was used]

### Specific Parts of Assignment
[Which parts of the assignment involved this tool]

### Conversation Log
[Use one block per round. Add more rounds as needed.]

#### Round 1
**Prompt:** [Exact prompt/question]
**AI Output:** [Summarize or paste the output]
**Verification/Edits:** [How you verified accuracy or modified the output]

#### Round 2
**Prompt:** [Exact prompt/question]
**AI Output:** [Summarize or paste the output]
**Verification/Edits:** [How you verified accuracy or modified the output]

#### Round 3 (optional)
**Prompt:** [Exact prompt/question]
**AI Output:** [Summarize or paste the output]
**Verification/Edits:** [How you verified accuracy or modified the output]

### Learning & Reflection
[What you learned from using this tool]

---

## AI Tool #3 (if applicable)

### Tool Name
[Tool name if you used a third AI tool]

### Version/Model
[Version/model information]

### Date(s) Used
[Dates when this tool was used]

### Specific Parts of Assignment
[Which parts of the assignment involved this tool]

### Conversation Log
[Use one block per round. Add more rounds as needed.]

#### Round 1
**Prompt:** [Exact prompt/question]
**AI Output:** [Summarize or paste the output]
**Verification/Edits:** [How you verified accuracy or modified the output]

#### Round 2
**Prompt:** [Exact prompt/question]
**AI Output:** [Summarize or paste the output]
**Verification/Edits:** [How you verified accuracy or modified the output]

#### Round 3 (optional)
**Prompt:** [Exact prompt/question]
**AI Output:** [Summarize or paste the output]
**Verification/Edits:** [How you verified accuracy or modified the output]

### Learning & Reflection
[What you learned from using this tool]

---

## Overall Reflection

[Provide an overall reflection on your AI usage for this assignment]
[Consider: How did AI tools help your learning? What did you understand better? How did you ensure the work remained your own?]

---

## Notes

- Be as specific and detailed as possible in your responses
- Keep the conversation rounds in order
- Include exact prompts and AI outputs when possible
- Explain how you verified and modified AI-generated content
- Reflect on what you learned through the AI interaction
