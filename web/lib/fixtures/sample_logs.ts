/**
 * Synthetic Test Fixtures for LogWatch Frontend [SYNTHETIC DATA]
 *
 * Guaranteed synthetic access logs for pedagogical demonstration.
 * In accordance with LogWatch data provenance rules, all records
 * are synthetic and marked [SYNTHETIC].
 */

export const DEFAULT_SYNTHETIC_LOG = `2026-10-09T10:00:01Z|/api/health|200|5|client-01
2026-10-09T10:00:03Z|/api/auth/login|200|45|client-02
2026-10-09T10:00:05Z|/api/snippets|200|32|client-03
2026-10-09T10:00:08Z|/api/snippets/1|200|18|client-02
2026-10-09T10:00:10Z|/api/snippets/2|200|22|client-04
2026-10-09T10:00:12Z|/api/snippets|201|85|client-05
2026-10-09T10:00:15Z|/api/auth/login|401|12|client-06
2026-10-09T10:00:18Z|/api/auth/login|200|48|client-06
2026-10-09T10:00:20Z|/api/snippets/search|200|110|client-03
2026-10-09T10:00:22Z|/api/snippets/99|404|15|client-07
2026-10-09T10:00:25Z|/api/users/profile|200|65|client-02
2026-10-09T10:00:28Z|/api/export/csv|200|450|client-08
2026-10-09T10:00:30Z|/api/snippets|500|320|client-09
2026-10-09T10:00:33Z|/api/health|200|4|client-01
2026-10-09T10:00:35Z|/api/snippets/3|200|28|client-04
2026-10-09T10:00:38Z|/api/snippets|200|35|client-10
2026-10-09T10:00:40Z|/api/auth/refresh|200|25|client-05
2026-10-09T10:00:42Z|/api/metrics|200|190|client-11
2026-10-09T10:00:45Z|/api/snippets/4|200|24|client-03
2026-10-09T10:00:48Z|/api/snippets/5|200|27|client-12
2026-10-09T10:00:50Z|/api/snippets|201|92|client-07
2026-10-09T10:00:52Z|/api/admin/reports|403|14|client-13
2026-10-09T10:00:55Z|/api/snippets/search|200|135|client-10
2026-10-09T10:00:58Z|/api/users/settings|200|55|client-02
2026-10-09T10:01:00Z|/api/health|200|6|client-01
2026-10-09T10:01:03Z|/api/export/json|200|580|client-14
2026-10-09T10:01:06Z|/api/snippets/6|200|30|client-06
2026-10-09T10:01:08Z|/api/auth/logout|204|8|client-05
2026-10-09T10:01:10Z|/api/snippets|503|1250|client-15
2026-10-09T10:01:13Z|/api/snippets/7|200|19|client-08
2026-10-09T10:01:15Z|/api/snippets|200|38|client-09
2026-10-09T10:01:18Z|/api/auth/login|200|42|client-05
2026-10-09T10:01:20Z|/api/snippets/8|200|21|client-11
2026-10-09T10:01:23Z|/api/users/avatar|200|210|client-02
2026-10-09T10:01:25Z|/api/snippets/search|200|98|client-12
2026-10-09T10:01:28Z|/api/snippets|201|88|client-03
2026-10-09T10:01:30Z|/api/health|200|5|client-01
2026-10-09T10:01:33Z|/api/snippets/9|200|26|client-04
2026-10-09T10:01:35Z|/api/snippets/10|200|23|client-07
2026-10-09T10:01:38Z|/api/snippets/batch|400|16|client-13
2026-10-09T10:01:40Z|/api/snippets|200|34|client-10
2026-10-09T10:01:43Z|/api/auth/refresh|200|22|client-06
2026-10-09T10:01:45Z|/api/snippets/11|200|25|client-14
2026-10-09T10:01:48Z|/api/snippets|500|410|client-15
2026-10-09T10:01:50Z|/api/snippets/12|200|29|client-08
2026-10-09T10:01:53Z|/api/snippets/13|200|20|client-09
2026-10-09T10:01:55Z|/api/export/csv|504|3500|client-02
2026-10-09T10:01:58Z|/api/health|200|5|client-01
2026-10-09T10:02:00Z|/api/snippets|200|36|client-11
2026-10-09T10:02:02Z|/api/auth/logout|204|9|client-06
`;

export const MALFORMED_SYNTHETIC_LOG = `2026-10-09T10:00:00Z|/api/snippets|200|42|client-A
not_a_valid_timestamp|/api/bad|200|50|client-B

2026-10-09T10:00:01Z|/api/missing_field|200|client-C
2026-10-09T10:00:02Z|/api/valid|200|95|client-D
2026-10-09T10:00:03Z|/api/bad_status|999|120|client-E

2026-10-09T10:00:04Z|/api/snippets/search|200|88|client-F
`;
