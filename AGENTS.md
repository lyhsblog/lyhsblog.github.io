# AGENTS.md

## Cursor Cloud specific instructions

This is a Docusaurus 3 static documentation site ("程序员LYH"). No databases, backend services, or Docker required.

### Quick reference

| Action | Command |
|--------|---------|
| Install deps | `yarn install` |
| Dev server | `yarn start` (port 3000) |
| Build | `yarn build` |
| Clear cache | `yarn clear` |

Standard scripts are in `package.json`; see `README.md` for full details.

### Notes

- The project has both `yarn.lock` and `package-lock.json`. Use **yarn** as the package manager (per README and user preference).
- `docusaurus.config.js` uses ESM (`import`/`export default`). Node >= 18 is required.
- The `punycode` deprecation warning on build/start is harmless and comes from a transitive dependency.
- No lint or test scripts are configured in `package.json`. Build (`yarn build`) is the primary correctness check.
- To bind the dev server on all interfaces (useful in cloud VMs), use `yarn start --host 0.0.0.0`.
