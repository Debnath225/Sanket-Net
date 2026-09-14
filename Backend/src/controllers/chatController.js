import { answerFromLiveData, isConversationalMessage, isLiveMonitoringQuestion } from "../ai/rag.js";

export function createChatController(store, generateAnswer) {
  return {
    ask: async (request, response, next) => {
      const question = String(request.body?.question || "").trim();
      const conversationId = request.body?.conversationId ? String(request.body.conversationId) : null;
      if (!question) return response.status(400).json({ error: "question_required" });
      if (question.length > 1000) return response.status(400).json({ error: "question_too_long" });
      if (conversationId && !/^[0-9a-f-]{36}$/i.test(conversationId)) return response.status(400).json({ error: "invalid_conversation_id" });
      try {
        const persistedContext = isConversationalMessage(question) || !isLiveMonitoringQuestion(question) ? [] : await store.searchSensorReadings(question);
        const result = await answerFromLiveData(question, store.state, generateAnswer, persistedContext);
        const savedConversationId = await store.saveChat({ conversationId, username: request.session.sub, question, answer: result.answer, sources: result.sources, provider: result.provider });
        return response.json({ ...result, conversationId: savedConversationId || conversationId || null });
      } catch (error) { if (error.code === "CONVERSATION_FORBIDDEN") return response.status(403).json({ error: "conversation_forbidden" }); return next(error); }
    },
    list: async (request, response, next) => { try { response.json(await store.listConversations(request.session.sub)); } catch (error) { next(error); } },
    messages: async (request, response, next) => { try { const conversation = await store.getConversation(request.params.id, request.session.sub); if (!conversation) return response.status(404).json({ error: "conversation_not_found" }); return response.json(conversation); } catch (error) { return next(error); } },
    remove: async (request, response, next) => { try { if (!await store.deleteConversation(request.params.id, request.session.sub)) return response.status(404).json({ error: "conversation_not_found" }); return response.status(204).end(); } catch (error) { return next(error); } },
  };
}
