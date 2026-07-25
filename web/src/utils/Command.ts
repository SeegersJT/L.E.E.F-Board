import type { CommandPhase } from '@/redux/types/Command.type'

export const deviceCommandKey = (deviceId: string, type: string) => `${deviceId}:${type}`

export const isCommandInFlight = (phase: CommandPhase | undefined): boolean =>
	phase === 'sending' || phase === 'pending' || phase === 'received' || phase === 'queued'

export const isCommandExecuting = (phase: CommandPhase | undefined): boolean =>
	phase === 'executing'
