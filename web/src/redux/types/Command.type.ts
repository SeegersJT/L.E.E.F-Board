export type CommandPhase =
	| 'sending'
	| 'pending'
	| 'received'
	| 'queued'
	| 'executing'
	| 'completed'
	| 'failed'
	| 'rejected'
	| 'expired'

export interface CommandRecord {
	deviceId: string
	type: string
	phase: CommandPhase
	error: string | null
}

export interface CommandsState {
	byId: Record<string, CommandRecord>
	latestKeyByDeviceAndType: Record<string, string>
}
