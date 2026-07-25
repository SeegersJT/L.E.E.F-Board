export const COMMAND_ACTIONS = {
	REQUEST_COMMAND: '[COMMAND] - REQUEST',
	SET_COMMAND_STARTED: '[COMMAND] - STARTED',
	SET_COMMAND_PHASE: '[COMMAND] - PHASE',
	COMMAND_ERROR: '[COMMAND] - ERROR',
	CLEAR_COMMAND: '[COMMAND] - CLEAR',
} as const

export const requestCommand = (payload: {
	deviceId: string
	type: string
	params?: Record<string, unknown>
}) => ({
	type: COMMAND_ACTIONS.REQUEST_COMMAND,
	payload,
})

export const commandStarted = (payload: { deviceId: string; type: string; commandId: string }) => ({
	type: COMMAND_ACTIONS.SET_COMMAND_STARTED,
	payload,
})

export const setCommandPhase = (payload: { commandId: string; phase: string }) => ({
	type: COMMAND_ACTIONS.SET_COMMAND_PHASE,
	payload,
})

export const commandError = (payload: { deviceId: string; type: string; message: string }) => ({
	type: COMMAND_ACTIONS.COMMAND_ERROR,
	payload,
})

export const clearCommand = (payload: { deviceId: string; type: string }) => ({
	type: COMMAND_ACTIONS.CLEAR_COMMAND,
	payload,
})
