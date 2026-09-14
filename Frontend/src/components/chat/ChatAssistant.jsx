import React, { useEffect, useRef, useState } from "react";
import { Bot, MessageCircle, Send, Sparkles, X } from "lucide-react";
import { useAuth } from "../../context/AuthContext.jsx";
import { endpoints } from "../../api/endpoints.js";

const QUICK_PROMPTS = [
  "Is there a flood or rain warning in my sector?",
  "What essential items belong in an emergency Go-Bag?",
  "What should I do if flash water levels rise rapidly?",
  "Where are safe relief assembly grounds located?",
];

export function ChatAssistant() {
  const { session } = useAuth();
  const token = session?.token;
  const [open, setOpen] = useState(false);
  const [question, setQuestion] = useState("");
  const [messages, setMessages] = useState([
    {
      role: "assistant",
      text: "Hello! I am Sankat AI, your disaster intelligence assistant. Ask me anything about current weather risks, water levels, emergency shelters, or safety steps.",
      sources: [],
    },
  ]);
  const [sending, setSending] = useState(false);
  const [conversationId, setConversationId] = useState(null);
  const messagesEndRef = useRef(null);
  const inputRef = useRef(null);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages, sending, open]);

  const toggle = () => {
    setOpen((prev) => {
      const next = !prev;
      if (next) setTimeout(() => inputRef.current?.focus(), 150);
      return next;
    });
  };

  const handleSend = async (textToSend) => {
    const text = (textToSend || question).trim();
    if (!text || sending || !token) return;

    setQuestion("");
    setMessages((prev) => [...prev, { role: "user", text }]);
    setSending(true);

    try {
      const result = await endpoints.askChat(text, conversationId, token);
      if (result.conversationId) setConversationId(result.conversationId);

      setMessages((prev) => [
        ...prev,
        {
          role: "assistant",
          text: result.answer || "I received live data but could not generate a response.",
          sources: result.sources || [],
        },
      ]);
    } catch {
      setMessages((prev) => [
        ...prev,
        {
          role: "assistant",
          text: "I am having trouble reaching the analysis service right now. Please check if the backend server is active.",
          sources: [],
        },
      ]);
    } finally {
      setSending(false);
    }
  };

  return (
    <div className="chat-assistant-container">
      {open && (
        <div className="chat-window-card" role="dialog" aria-label="Sankat AI Assistant">
          <header className="chat-window-head">
            <div className="chat-head-left">
              <div className="chat-bot-avatar">
                <Bot size={18} />
              </div>
              <div>
                <strong className="chat-title">Sankat AI Assistant</strong>
                <span className="chat-status-pill">
                  <span className="dot" /> Grounded Operational RAG
                </span>
              </div>
            </div>
            <button className="chat-close-btn" onClick={toggle} aria-label="Close chat">
              <X size={18} />
            </button>
          </header>

          <div className="chat-messages-scroll">
            {messages.map((msg, i) => (
              <div key={i} className={`chat-bubble-row ${msg.role}`}>
                <div className="chat-bubble">
                  <p>{msg.text}</p>
                  {msg.sources && msg.sources.length > 0 && (
                    <details className="chat-sources-collapse">
                      <summary>
                        <Sparkles size={11} />
                        <span>Verified Live Sources ({msg.sources.length})</span>
                      </summary>
                      <div className="sources-list">
                        {msg.sources.map((src, idx) => (
                          <div key={idx} className="source-item">
                            <span className="source-kind">{src.kind}</span>
                            <span className="source-summary">{src.summary}</span>
                          </div>
                        ))}
                      </div>
                    </details>
                  )}
                </div>
              </div>
            ))}

            {sending && (
              <div className="chat-bubble-row assistant">
                <div className="chat-bubble typing">
                  <span className="dot" />
                  <span className="dot" />
                  <span className="dot" />
                </div>
              </div>
            )}
            <div ref={messagesEndRef} />
          </div>

          {messages.length <= 2 && (
            <div className="chat-quick-prompts">
              <span className="quick-label">Suggested questions:</span>
              <div className="prompt-pills">
                {QUICK_PROMPTS.map((prompt, idx) => (
                  <button
                    key={idx}
                    type="button"
                    className="prompt-pill"
                    onClick={() => handleSend(prompt)}
                  >
                    {prompt}
                  </button>
                ))}
              </div>
            </div>
          )}

          <form
            className="chat-input-bar"
            onSubmit={(e) => {
              e.preventDefault();
              handleSend();
            }}
          >
            <input
              ref={inputRef}
              type="text"
              value={question}
              onChange={(e) => setQuestion(e.target.value)}
              placeholder="Ask about live risk, water level, emergency..."
              maxLength={600}
              disabled={sending}
            />
            <button
              type="submit"
              className="chat-send-btn"
              disabled={!question.trim() || sending}
              aria-label="Send message"
            >
              <Send size={16} />
            </button>
          </form>
        </div>
      )}

      <button
        className={`chat-toggle-fab ${open ? "open" : ""}`}
        onClick={toggle}
        aria-label="Open Sankat AI Safety Assistant"
      >
        <MessageCircle size={22} />
        <span className="fab-label">Ask Sankat AI</span>
      </button>
    </div>
  );
}
