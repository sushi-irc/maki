/*
 * Copyright (c) 2008 Michael Kuhn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "maki.h"

struct maki_cache
{
	gpointer (*value_new) (gpointer, gpointer);
	void (*value_free) (gpointer);
	gpointer value_data;

	GHashTable* hash_table;
};

struct maki_cache_item
{
	gint ref_count;
	gpointer key;
	gpointer value;
};

typedef struct maki_cache_item makiCacheItem;

static makiCacheItem* maki_cache_item_new (gpointer key, gpointer value)
{
	makiCacheItem* item;

	item = g_new(makiCacheItem, 1);
	item->ref_count = 1;
	item->key = key;
	item->value = value;

	return item;
}

static void maki_cache_item_free (makiCacheItem* item)
{
	g_free(item);
}

makiCache* maki_cache_new (gpointer (*value_new) (gpointer, gpointer), void (*value_free) (gpointer), gpointer value_data)
{
	makiCache* cache;

	g_return_val_if_fail(value_new != NULL, NULL);
	g_return_val_if_fail(value_free != NULL, NULL);

	cache = g_new(makiCache, 1);
	cache->value_new = value_new;
	cache->value_free = value_free;
	cache->value_data = value_data;
	cache->hash_table = g_hash_table_new_full(maki_str_hash, maki_str_equal, NULL, NULL);

	return cache;
}

void maki_cache_free (makiCache* cache)
{
	g_return_if_fail(cache != NULL);

	g_hash_table_destroy(cache->hash_table);
	g_free(cache);
}

gpointer maki_cache_insert (makiCache* cache, gpointer key)
{
	gpointer value;
	makiCacheItem* item;

	g_return_val_if_fail(cache != NULL, NULL);

	if ((item = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		item->ref_count++;

		return item->value;
	}
	else
	{
		key = g_strdup(key);
		value = (*cache->value_new)(key, cache->value_data);

		item = maki_cache_item_new(key, value);

		g_hash_table_insert(cache->hash_table, key, item);

		return value;
	}
}

void maki_cache_remove (makiCache* cache, gpointer key)
{
	makiCacheItem* item;

	g_return_if_fail(cache != NULL);

	if ((item = g_hash_table_lookup(cache->hash_table, key)) != NULL)
	{
		item->ref_count--;

		if (item->ref_count == 0)
		{
			g_hash_table_remove(cache->hash_table, item->key);
			(*cache->value_free)(item->value);
			g_free(item->key);
			maki_cache_item_free(item);
		}
	}
}
