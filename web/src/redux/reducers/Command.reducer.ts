import { COMMAND_ACTIONS } from '../actions/Command.action'
import type { CommandPhase, CommandsState } from '../types/Command.type'

const initialState: CommandsState = { byId: {}, latestKeyByDeviceAndType: {} }

const key = (deviceId: string, type: string) => `${deviceId}:${type}`

type Action = { type: string; payload?: unknown }

export const CommandsReducer = (state = initialState, action: Action): CommandsState => {
	switch (action.type) {
		case COMMAND_ACTIONS.SET_COMMAND_STARTED: {
			const { deviceId, type, commandId } = action.payload as {
				deviceId: string
				type: string
				commandId: string
			}
			return {
				byId: {
					...state.byId,
					[commandId]: { deviceId, type, phase: 'pending', error: null },
				},
				latestKeyByDeviceAndType: {
					...state.latestKeyByDeviceAndType,
					[key(deviceId, type)]: commandId,
				},
			}
		}

		case COMMAND_ACTIONS.SET_COMMAND_PHASE: {
			const { commandId, phase } = action.payload as { commandId: string; phase: string }
			const existing = state.byId[commandId]
			if (!existing) return state
			return {
				...state,
				byId: { ...state.byId, [commandId]: { ...existing, phase: phase as CommandPhase } },
			}
		}

		case COMMAND_ACTIONS.COMMAND_ERROR: {
			const { deviceId, type, message } = action.payload as {
				deviceId: string
				type: string
				message: string
			}
			const localId = `local-${deviceId}-${type}-${Date.now()}`
			return {
				byId: {
					...state.byId,
					[localId]: { deviceId, type, phase: 'failed', error: message },
				},
				latestKeyByDeviceAndType: {
					...state.latestKeyByDeviceAndType,
					[key(deviceId, type)]: localId,
				},
			}
		}

		case COMMAND_ACTIONS.CLEAR_COMMAND: {
			const { deviceId, type } = action.payload as { deviceId: string; type: string }
			const mapKey = key(deviceId, type)
			const commandId = state.latestKeyByDeviceAndType[mapKey]
			if (!commandId) return state

			const restById = Object.fromEntries(
				Object.entries(state.byId).filter(([id]) => id !== commandId)
			)
			const restKeys = Object.fromEntries(
				Object.entries(state.latestKeyByDeviceAndType).filter(([k]) => k !== mapKey)
			)
			return { byId: restById, latestKeyByDeviceAndType: restKeys }
		}

		default:
			return state
	}
}
